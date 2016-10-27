#pragma once

#include "../VertexShader.h"
#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

//This buffer should contain variables that never, or rarely change
BEGIN_UNIFORM_BUFFER_STRUCT(FSnowPixelShaderConstantParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, ClimateDataDimension)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CellsDimensionX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CellsDimensionY)
END_UNIFORM_BUFFER_STRUCT(FSnowPixelShaderConstantParameters)

//This buffer is for variables that change very often (each frame for example)
BEGIN_UNIFORM_BUFFER_STRUCT(FSnowPixelShaderVariableParameters, )
END_UNIFORM_BUFFER_STRUCT(FSnowPixelShaderVariableParameters)

typedef TUniformBufferRef<FSnowPixelShaderConstantParameters> FSnowPixelShaderConstantParametersRef;
typedef TUniformBufferRef<FSnowPixelShaderVariableParameters> FSnowPixelShaderVariableParametersRef;


/**
* A simple passthrough vertexshader that we will use.
*/
class FSnowVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSnowVertexShader, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FSnowVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{}
	FSnowVertexShader() {}
};

/**
* This class is what encapsulates the shader in the engine.
* It is the main bridge between the HLSL located in the engine directory
* and the engine itself.
*/
class FSnowPixelShaderDeclaration : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSnowPixelShaderDeclaration, Global);

public:

	FSnowPixelShaderDeclaration() {}

	explicit FSnowPixelShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);

		Ar << SnowMap;
		Ar << MaxSnow;

		return bShaderHasOutdatedParams;
	}

	//This function is required to let us bind our runtime surface to the shader using an SRV.
	void SetParameters(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef SnowMap, FShaderResourceViewRHIRef MaxSnowSRV);
	//This function is required to bind our constant / uniform buffers to the shader.
	void SetUniformBuffers(FRHICommandList& RHICmdList, FSnowPixelShaderConstantParameters& ConstantParameters, FSnowPixelShaderVariableParameters& VariableParameters);
	//This is used to clean up the buffer binds after each invocation to let them be changed and used elsewhere if needed.
	void UnbindBuffers(FRHICommandList& RHICmdList);

private:
	FShaderResourceParameter SnowMap;

	FShaderResourceParameter MaxSnow;
};

