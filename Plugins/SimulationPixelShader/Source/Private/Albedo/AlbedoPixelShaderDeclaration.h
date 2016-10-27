#pragma once

#include "../VertexShader.h"
#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

//This buffer should contain variables that never, or rarely change
BEGIN_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderConstantParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, ClimateDataDimension)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CellsDimensionX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CellsDimensionY)
END_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderConstantParameters)

//This buffer is for variables that change very often (each frame for example)
BEGIN_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderVariableParameters, )
END_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderVariableParameters)

typedef TUniformBufferRef<FAlbedoPixelShaderConstantParameters> FAlbedoPixelShaderConstantParametersRef;
typedef TUniformBufferRef<FAlbedoPixelShaderVariableParameters> FAlbedoPixelShaderVariableParametersRef;


/**
* A simple passthrough vertexshader that we will use.
*/
class FAlbedoVertexShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAlbedoVertexShader, Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return true; }

	FAlbedoVertexShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FGlobalShader(Initializer)
	{}
	FAlbedoVertexShader() {}
};

/**
* This class is what encapsulates the shader in the engine.
* It is the main bridge between the HLSL located in the engine directory
* and the engine itself.
*/
class FAlbedoPixelShaderDeclaration : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FAlbedoPixelShaderDeclaration, Global);

public:

	FAlbedoPixelShaderDeclaration() {}

	explicit FAlbedoPixelShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);

		Ar << AlbedoMap;

		return bShaderHasOutdatedParams;
	}

	//This function is required to let us bind our runtime surface to the shader using an SRV.
	void SetParameters(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef AlbedoMap);
	//This function is required to bind our constant / uniform buffers to the shader.
	void SetUniformBuffers(FRHICommandList& RHICmdList, FAlbedoPixelShaderConstantParameters& ConstantParameters, FAlbedoPixelShaderVariableParameters& VariableParameters);
	//This is used to clean up the buffer binds after each invocation to let them be changed and used elsewhere if needed.
	void UnbindBuffers(FRHICommandList& RHICmdList);

private:
	FShaderResourceParameter AlbedoMap;

};

