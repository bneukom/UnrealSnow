/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2015 Fredrik Lindh
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

#pragma once

#include "Private/PixelShaderDeclaration.h"
#include "RWStructuredBuffer.h"

/***************************************************************************/
/* This class demonstrates how to use the pixel shader we have declared.   */
/* Most importantly which RHI functions are needed to call and how to get  */
/* some interesting output.                                                */
/***************************************************************************/
class SIMULATIONPIXELSHADER_API FSimulationPixelShader
{
public:
	FSimulationPixelShader(ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FSimulationPixelShader();

	/**
	* Let the user change render target during runtime if they want to.
	* @param RenderTarget - This is the output render target
	*/
	void ExecutePixelShader(UTextureRenderTarget2D* RenderTarget);

	/**
	* Only execute this from the render thread
	*/
	void ExecutePixelShaderInternal();

	/** Initializes the simulation with the correct input data. */
	void Initialize(FRWStructuredBuffer* SnowBuffer, FRWStructuredBuffer* MaxSnowBuffer, int32 CellsDimension);

private:
	bool bIsPixelShaderExecuting;
	bool bMustRegenerateSRV;
	bool bIsUnloading;
	bool bSave;

	FPixelShaderConstantParameters ConstantParameters;
	FPixelShaderVariableParameters VariableParameters;
	ERHIFeatureLevel::Type FeatureLevel;

	/** Main texture */
	FTexture2DRHIRef CurrentTexture;
	UTextureRenderTarget2D* CurrentRenderTarget;

	/** Input snow map buffer. */
	FRWStructuredBuffer* SnowInputBuffer;

	/** Input snow max buffer. */
	FRWStructuredBuffer* MaxSnowInputBuffer;
};
