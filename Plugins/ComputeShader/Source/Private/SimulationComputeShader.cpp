
#include "ComputeShaderPrivatePCH.h"
#include "WeatherData.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 4 // This has to be the same as in the compute shaders spec [X, X, 1]

FSimulationComputeShader::FSimulationComputeShader(ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	IsComputeShaderExecuting = false;
	IsUnloading = false;
}

FSimulationComputeShader::~FSimulationComputeShader()
{
	IsUnloading = true;
}

void FSimulationComputeShader::Initialize(TResourceArray<FComputeShaderSimulationCell>& Cells, float k_e, float k_m, float TMeltA, float TMeltB, float TSnowA, float TSnowB, int32 CellsDimension, int32 WeatherDataResolution)
{
	NumCells = Cells.Num();

	//There are only a few different texture formats we can use if we want to use the output texture as input in a pixel shader later
	//I would have loved to go with the R8G8B8A8_UNORM approach, but unfortunately, it seems UE4 does not support this in an obvious way, which is why I chose the UINT format using packing instead
	//There is some excellent information on this topic in the following links:
	//http://www.gamedev.net/topic/605356-r8g8b8a8-texture-format-in-compute-shader/
	//https://msdn.microsoft.com/en-us/library/ff728749(v=vs.85).aspx
	FRHIResourceCreateInfo CreateInfo;

	Texture = RHICreateTexture2D(CellsDimension, CellsDimension, PF_B8G8R8A8, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	TextureUAV = RHICreateUnorderedAccessView(Texture);

	// Create input data buffers
	SimulationCellsBuffer = new FRWStructuredBuffer();
	SimulationCellsBuffer->Initialize(sizeof(FComputeShaderSimulationCell), CellsDimension * CellsDimension, &Cells, 0, true, false);

	ClimateDataBuffer = new FRWStructuredBuffer();
	ClimateDataBuffer->Initialize(sizeof(FWeatherData), CellsDimension * CellsDimension, nullptr, 0, true, false);

	// Fill constant parameters
	ConstantParameters.CellsDimension = CellsDimension;
	ConstantParameters.WeatherDataResolution = WeatherDataResolution;
	ConstantParameters.ThreadGroupCountX = Texture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION;
	ConstantParameters.ThreadGroupCountY = Texture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION;
	ConstantParameters.k_e = k_e;
	ConstantParameters.k_m = k_m;
	ConstantParameters.TMeltA = TMeltA;
	ConstantParameters.TMeltB = TMeltB;
	ConstantParameters.TSnowA = TSnowA;
	ConstantParameters.TSnowB = TSnowB;

	VariableParameters = FComputeShaderVariableParameters();
}

void FSimulationComputeShader::ExecuteComputeShader(int TimeStep, TResourceArray<FWeatherData>* ClimateData)
{
	// Skip this execution round if we are already executing
	if (IsUnloading || IsComputeShaderExecuting) return;

	IsComputeShaderExecuting = true;

	// Now set our runtime parameters
	VariableParameters.TimeStep = TimeStep;

	// This macro sends the function we declare inside to be run on the render thread. What we do is essentially just send this class and tell the render thread to run the internal render function as soon as it can.
	// I am still not 100% Certain on the thread safety of this, if you are getting crashes, depending on how advanced code you have in the start of the ExecutePixelShader function, you might have to use a lock :)
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FComputeShaderRunner,
		FSimulationComputeShader*, ComputeShader, this, 
		TResourceArray<FWeatherData>*, Data, ClimateData,
		{
			ComputeShader->ExecuteComputeShaderInternal(Data);
		}
	);
}

void FSimulationComputeShader::ExecuteComputeShaderInternal(TResourceArray<FWeatherData>* ClimateData)
{
	check(IsInRenderingThread());

	// If we are about to unload, so just clean up the UAV
	if (IsUnloading)
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

	// Update climate data buffer
	uint32* Buffer = (uint32*)RHICmdList.LockStructuredBuffer(ClimateDataBuffer->Buffer, 0, ClimateDataBuffer->NumBytes, RLM_ReadOnly);
	auto Data = ClimateData->GetData();
	FMemory::Memcpy(Buffer, ClimateData, ClimateDataBuffer->NumBytes);
	RHICmdList.UnlockStructuredBuffer(ClimateDataBuffer->Buffer);

	// Compute shader calculation
	TShaderMapRef<FComputeShaderDeclaration> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	// Set inputs/outputs and dispatch compute shader
	ComputeShader->SetParameters(RHICmdList, TextureUAV, SimulationCellsBuffer->UAV, ClimateDataBuffer->UAV);

	ComputeShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);
	DispatchComputeShader(RHICmdList, *ComputeShader, Texture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION, Texture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION, 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	IsComputeShaderExecuting = false;

	// Copy results from GPU
	if (Debug)
	{
		TArray<FComputeShaderSimulationCell> SimulationCells;
		SimulationCells.Reserve(NumCells);
		SimulationCells.AddUninitialized(NumCells);
		uint32* Buffer = (uint32*)RHICmdList.LockStructuredBuffer(SimulationCellsBuffer->Buffer, 0, SimulationCellsBuffer->NumBytes, RLM_ReadOnly);
		FMemory::Memcpy(SimulationCells.GetData(), Buffer, SimulationCellsBuffer->NumBytes);
		RHICmdList.UnlockStructuredBuffer(SimulationCellsBuffer->Buffer);
	}
	/*
	uint32 Stride = 0;
	void* TextureData = RHICmdList.LockTexture2D(Texture, 0, EResourceLockMode::RLM_ReadOnly, Stride, true);

	Texture->GetTexture2D();
	auto t = UTexture2D::CreateTransient(NumCells, NumCells, PF_B8G8R8A8);
	*/
}

