
#include "../PixelShaderPrivatePCH.h"
#include "AlbedoPixelShaderDeclaration.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

//These are needed to actually implement the constant buffers so they are available inside our shader
//They also need to be unique over the entire solution since they can in fact be accessed from any shader
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderConstantParameters, TEXT("SimulationPSConstants"))
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FAlbedoPixelShaderVariableParameters, TEXT("SimulationPSVariables"))

FAlbedoPixelShaderDeclaration::FAlbedoPixelShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
: FGlobalShader(Initializer)
{
	AlbedoMap.Bind(Initializer.ParameterMap, TEXT("AlbedoInputBuffer"));
}

void FAlbedoPixelShaderDeclaration::SetUniformBuffers(FRHICommandList& RHICmdList, FAlbedoPixelShaderConstantParameters& ConstantParameters, FAlbedoPixelShaderVariableParameters& VariableParameters)
{
	FAlbedoPixelShaderConstantParametersRef ConstantParametersBuffer;
	FAlbedoPixelShaderVariableParametersRef VariableParametersBuffer;

	ConstantParametersBuffer = FAlbedoPixelShaderConstantParametersRef::CreateUniformBufferImmediate(ConstantParameters, UniformBuffer_SingleDraw);
	VariableParametersBuffer = FAlbedoPixelShaderVariableParametersRef::CreateUniformBufferImmediate(VariableParameters, UniformBuffer_SingleDraw);

	SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FAlbedoPixelShaderConstantParameters>(), ConstantParametersBuffer);
	SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FAlbedoPixelShaderVariableParameters>(), VariableParametersBuffer);
}

void FAlbedoPixelShaderDeclaration::SetParameters(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef AlbedoMapSRV)
{
	FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

	if (AlbedoMap.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, AlbedoMap.GetBaseIndex(), AlbedoMapSRV);

}

void FAlbedoPixelShaderDeclaration::UnbindBuffers(FRHICommandList& RHICmdList)
{
	FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

	if (AlbedoMap.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, AlbedoMap.GetBaseIndex(), FShaderResourceViewRHIRef());

}

//This is what will instantiate the shader into the engine from the engine/Shaders folder
//                      ShaderType               ShaderFileName     Shader function name            Type
IMPLEMENT_SHADER_TYPE(, FAlbedoVertexShader, TEXT("AlbedoPixelShader"), TEXT("MainVertexShader"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FAlbedoPixelShaderDeclaration, TEXT("AlbedoPixelShader"), TEXT("MainPixelShader"), SF_Pixel);

// Needed to make sure the plugin works
IMPLEMENT_MODULE(FDefaultModuleImpl, PixelShader)