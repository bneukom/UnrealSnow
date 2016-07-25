#include "ComputeShaderPrivatePCH.h"


#define NUM_THREADS_PER_GROUP_DIMENSION 4 // This has to be the same as in the compute shader's spec [X, X, 1]

FSimulationComputeShader::FSimulationComputeShader(float SimulationSpeed, int32 SizeX, int32 SizeY, ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	ConstantParameters.SimulationSpeed = SimulationSpeed;

	VariableParameters = FComputeShaderVariableParameters();

	IsComputeShaderExecuting = false;
	IsUnloading = false;

	//There are only a few different texture formats we can use if we want to use the output texture as input in a pixel shader later
	//I would have loved to go with the R8G8B8A8_UNORM approach, but unfortunately, it seems UE4 does not support this in an obvious way, which is why I chose the UINT format using packing instead :)
	//There is some excellent information on this topic in the following links:
    //http://www.gamedev.net/topic/605356-r8g8b8a8-texture-format-in-compute-shader/
	//https://msdn.microsoft.com/en-us/library/ff728749(v=vs.85).aspx
	FRHIResourceCreateInfo CreateInfo;

	Texture = RHICreateTexture2D(SizeX, SizeY, PF_R32_UINT, 1, 1, TexCreate_ShaderResource | TexCreate_UAV, CreateInfo);
	TextureUAV = RHICreateUnorderedAccessView(Texture);
	
	SimulationCellsBuffer = new FRWBufferStructured();
	SimulationCellsBuffer->Initialize(sizeof(FComputeShaderSimulationCell), SizeX * SizeY, 0, true, false);
}

FSimulationComputeShader::~FSimulationComputeShader()
{
	IsUnloading = true;

}

void FSimulationComputeShader::ExecuteComputeShader(float TotalElapsedTimeSeconds)
{

	if (IsUnloading || IsComputeShaderExecuting) //Skip this execution round if we are already executing
	{
		return;
	}

	IsComputeShaderExecuting = true;

	// Now set our runtime parameters!
	VariableParameters.TotalTimeElapsedSeconds = TotalElapsedTimeSeconds;

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
	
	if (IsUnloading) //If we are about to unload, so just clean up the UAV :)
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

	/* Get global RHI command list */
	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	/** Compute shader calculation */
	TShaderMapRef<FComputeShaderDeclaration> ComputeShader(GetGlobalShaderMap(FeatureLevel));
	
	RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	// @TODO https://forums.unrealengine.com/showthread.php?48948-Get-Data-back-from-Compute-Shader

	/* Set inputs/outputs and dispatch compute shader */
	ComputeShader->SetParameters(RHICmdList, TextureUAV, SimulationCellsBuffer->UAV);

	ComputeShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);
	DispatchComputeShader(RHICmdList, *ComputeShader, Texture->GetSizeX() / NUM_THREADS_PER_GROUP_DIMENSION, Texture->GetSizeY() / NUM_THREADS_PER_GROUP_DIMENSION, 1);
	ComputeShader->UnbindBuffers(RHICmdList);

	IsComputeShaderExecuting = false;
}

