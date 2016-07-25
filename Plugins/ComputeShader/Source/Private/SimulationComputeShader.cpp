#include "ComputeShaderPrivatePCH.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 4 // This has to be the same as in the compute shaders spec [X, X, 1]

FSimulationComputeShader::FSimulationComputeShader(float SimulationSpeed, int32 SizeX, int32 SizeY, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	IsComputeShaderExecuting = false;
	IsUnloading = false;
	NumCells = SizeX * SizeY;

	//There are only a few different texture formats we can use if we want to use the output texture as input in a pixel shader later
	//I would have loved to go with the R8G8B8A8_UNORM approach, but unfortunately, it seems UE4 does not support this in an obvious way, which is why I chose the UINT format using packing instead :)
	//There is some excellent information on this topic in the following links:
    //http://www.gamedev.net/topic/605356-r8g8b8a8-texture-format-in-compute-shader/
	//https://msdn.microsoft.com/en-us/library/ff728749(v=vs.85).aspx
	FRHIResourceCreateInfo CreateInfo;

	Texture = RHICreateTexture2D(SizeX, SizeY, PF_R32_UINT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	TextureUAV = RHICreateUnorderedAccessView(Texture);
	
	TResourceArray<FComputeShaderSimulationCell> SimulationCells;
	SimulationCells.Init(FComputeShaderSimulationCell(0, 0, 0, 0, 0.5f), SizeX * SizeY);
	SimulationCellsBuffer = new FRWStructuredBuffer();
	SimulationCellsBuffer->Initialize(sizeof(FComputeShaderSimulationCell), SizeX * SizeY, &SimulationCells, 0, true, false);

	TResourceArray<float> TemperatureData;
	TemperatureData.Init(1.0f, SizeX * SizeY);
	TemperatureDataBuffer = new FRWStructuredBuffer();
	TemperatureDataBuffer->Initialize(sizeof(float), SizeX * SizeY, &TemperatureData, 0, true, false);

	ConstantParameters.SimulationSpeed = SimulationSpeed;
	ConstantParameters.ThreadGroupCountX = Texture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION;
	ConstantParameters.ThreadGroupCountY = Texture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION;

	VariableParameters = FComputeShaderVariableParameters();
}

FSimulationComputeShader::~FSimulationComputeShader()
{
	IsUnloading = true;
}

void FSimulationComputeShader::ExecuteComputeShader(float TotalElapsedTimeSeconds)
{
	// Skip this execution round if we are already executing
	if (IsUnloading || IsComputeShaderExecuting) return;

	IsComputeShaderExecuting = true;

	// Now set our runtime parameters!
	VariableParameters.TimeStep = TotalElapsedTimeSeconds;

	// This macro sends the function we declare inside to be run on the render thread. What we do is essentially just send this class and tell the render thread to run the internal render function as soon as it can.
	// I am still not 100% Certain on the thread safety of this, if you are getting crashes, depending on how advanced code you have in the start of the ExecutePixelShader function, you might have to use a lock :)
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		FComputeShaderRunner,
		FSimulationComputeShader*, ComputeShader, this,
		{
			ComputeShader->ExecuteComputeShaderInternal();
		}
	);


}

void FSimulationComputeShader::ExecuteComputeShaderInternal()
{
	check(IsInRenderingThread());
	
	if (IsUnloading) // If we are about to unload, so just clean up the UAV :)
	{
		if (NULL != TextureUAV)
		{
			TextureUAV.SafeRelease();
			TextureUAV = NULL;
		}
		if (NULL != SimulationCellsBuffer)
		{
			SimulationCellsBuffer->Release();
			delete SimulationCellsBuffer;
		}

		return;
	}

	// Get global RHI command list
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	// Compute shader calculation
	TShaderMapRef<FComputeShaderDeclaration> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	// Set inputs/outputs and dispatch compute shader
	ComputeShader->SetParameters(RHICmdList, TextureUAV, SimulationCellsBuffer->UAV, TemperatureDataBuffer->UAV);

	ComputeShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);
	DispatchComputeShader(RHICmdList, *ComputeShader, Texture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION, Texture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION, 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	IsComputeShaderExecuting = false;

	// Download results from GPU
	TArray<FComputeShaderSimulationCell> SimulationCells;
	SimulationCells.Reserve(NumCells);
	SimulationCells.AddUninitialized(NumCells);
	uint32* Buffer = (uint32*) RHICmdList.LockStructuredBuffer(SimulationCellsBuffer->Buffer, 0, SimulationCellsBuffer->NumBytes, RLM_ReadOnly);
	FMemory::Memcpy(Buffer, SimulationCells.GetData(), SimulationCellsBuffer->NumBytes);
	RHICmdList.UnlockStructuredBuffer(SimulationCellsBuffer->Buffer);

	// @TODO TESTING OUTPUT TEXTURE
	TArray<FColor> Bitmap;
	
	//To access our resource we do a custom read using lockrect
	uint32 LolStride = 0;
	char* TextureDataPtr = (char*)RHICmdList.LockTexture2D(Texture, 0, EResourceLockMode::RLM_ReadOnly, LolStride, false);

	for (uint32 Row = 0; Row < Texture->GetSizeY(); ++Row)
	{
		uint32* PixelPtr = (uint32*)TextureDataPtr;

		//Since we are using our custom UINT format, we need to unpack it here to access the actual colors
		for (uint32 Col = 0; Col < Texture->GetSizeX(); ++Col)
		{
			uint32 EncodedPixel = *PixelPtr;
			uint8 r = (EncodedPixel & 0x000000FF);
			uint8 g = (EncodedPixel & 0x0000FF00) >> 8;
			uint8 b = (EncodedPixel & 0x00FF0000) >> 16;
			uint8 a = (EncodedPixel & 0xFF000000) >> 24;
			Bitmap.Add(FColor(r, g, b, a));

			PixelPtr++;
		}

		// move to next row:
		TextureDataPtr += LolStride;
	}

	RHICmdList.UnlockTexture2D(Texture, 0, false);

	// if the format and texture type is supported
	if (Bitmap.Num())
	{
		// Create screenshot folder if not already present.
		IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

		const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("VisualizeTexture"));

		uint32 ExtendXWithMSAA = Bitmap.Num() / Texture->GetSizeY();

		// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
		FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, Texture->GetSizeY(), Bitmap.GetData());

		UE_LOG(LogConsoleResponse, Display, TEXT("Content was saved to \"%s\""), *FPaths::ScreenShotDir());
	}
	else
	{
		UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
	}
}

