
#include "PixelShaderPrivatePCH.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

//These are needed to actually implement the constant buffers so they are available inside our shader
//They also need to be unique over the entire solution since they can in fact be accessed from any shader
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FPixelShaderConstantParameters, TEXT("SimulationPSConstants"))
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FPixelShaderVariableParameters, TEXT("SimulationPSVariables"))

FPixelShaderDeclaration::FPixelShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
: FGlobalShader(Initializer)
{
	SnowMap.Bind(Initializer.ParameterMap, TEXT("SnowInputBuffer"));
	MaxSnow.Bind(Initializer.ParameterMap, TEXT("MaxSnowInputBuffer"));
}

void FPixelShaderDeclaration::SetUniformBuffers(FRHICommandList& RHICmdList, FPixelShaderConstantParameters& ConstantParameters, FPixelShaderVariableParameters& VariableParameters)
{
	FPixelShaderConstantParametersRef ConstantParametersBuffer;
	FPixelShaderVariableParametersRef VariableParametersBuffer;

	ConstantParametersBuffer = FPixelShaderConstantParametersRef::CreateUniformBufferImmediate(ConstantParameters, UniformBuffer_SingleDraw);
	VariableParametersBuffer = FPixelShaderVariableParametersRef::CreateUniformBufferImmediate(VariableParameters, UniformBuffer_SingleDraw);

	SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FPixelShaderConstantParameters>(), ConstantParametersBuffer);
	SetUniformBufferParameter(RHICmdList, GetPixelShader(), GetUniformBufferParameter<FPixelShaderVariableParameters>(), VariableParametersBuffer);
}

void FPixelShaderDeclaration::SetParameters(FRHICommandList& RHICmdList, FShaderResourceViewRHIRef SnowMapSRV, FShaderResourceViewRHIRef MaxSnowSRV)
{
	FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

	if (SnowMap.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, SnowMap.GetBaseIndex(), SnowMapSRV);
	if (MaxSnow.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, MaxSnow.GetBaseIndex(), MaxSnowSRV);
}

void FPixelShaderDeclaration::UnbindBuffers(FRHICommandList& RHICmdList)
{
	FPixelShaderRHIParamRef PixelShaderRHI = GetPixelShader();

	if (SnowMap.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, SnowMap.GetBaseIndex(), FShaderResourceViewRHIRef());
	if (MaxSnow.IsBound())
		RHICmdList.SetShaderResourceViewParameter(PixelShaderRHI, MaxSnow.GetBaseIndex(), FShaderResourceViewRHIRef());
}

//This is what will instantiate the shader into the engine from the engine/Shaders folder
//                      ShaderType               ShaderFileName     Shader function name            Type
IMPLEMENT_SHADER_TYPE(, FVertexShaderExample, TEXT("DegreeDaySnowSimulationPixelShader"), TEXT("MainVertexShader"), SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FPixelShaderDeclaration, TEXT("DegreeDaySnowSimulationPixelShader"), TEXT("MainPixelShader"), SF_Pixel);

//Needed to make sure the plugin works
IMPLEMENT_MODULE(FDefaultModuleImpl, PixelShader)