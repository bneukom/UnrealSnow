
#pragma once

#include "Private/Albedo/AlbedoPixelShaderDeclaration.h"
#include "RWStructuredBuffer.h"

/***************************************************************************/
/* This class demonstrates how to use the pixel shader we have declared.   */
/* Most importantly which RHI functions are needed to call and how to get  */
/* some interesting output.                                                */
/***************************************************************************/
class SIMULATIONPIXELSHADER_API FAlbedoPixelShader
{
public:
	FAlbedoPixelShader(ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FAlbedoPixelShader();

	/**
	* Let the user change render target during runtime if they want to.
	* @param RenderTarget - This is the output render target
	*/
	void ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, bool Save);

	/**
	* Only execute this from the render thread
	*/
	void ExecutePixelShaderInternal(bool Save);

	/** Initializes the simulation with the correct input data. */
	void Initialize(FRWStructuredBuffer* AlbedoBuffer, int32 CellsDimensionX, int32 CellsDimensionY);

private:
	bool bIsPixelShaderExecuting;
	bool bMustRegenerateSRV;
	bool bIsUnloading;
	bool bSave;

	FAlbedoPixelShaderConstantParameters ConstantParameters;
	FAlbedoPixelShaderVariableParameters VariableParameters;
	ERHIFeatureLevel::Type FeatureLevel;

	/** Main texture */
	FTexture2DRHIRef CurrentTexture;
	UTextureRenderTarget2D* CurrentRenderTarget;

	/** Input albedo buffer. */
	FRWStructuredBuffer* AlbedoInputBuffer;
};
