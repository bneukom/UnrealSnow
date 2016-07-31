#include "ComputeShaderPrivatePCH.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"

// These are needed to actually implement the constant buffers so they are available inside our shader
// They also need to be unique over the entire solution since they can in fact be accessed from any shader
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FComputeShaderConstantParameters, TEXT("CSConstants"))
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, TEXT("CSVariables"))

FComputeShaderDeclaration::FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
: FGlobalShader(Initializer)
{
	// This call is what lets the shader system know that the surface OutputSurface is going to be available in the shader. The second parameter is the name it will be known by in the shader
	OutputSurface.Bind(Initializer.ParameterMap, TEXT("OutputSurface"));
	SimulationCells.Bind(Initializer.ParameterMap, TEXT("SimulationCellsBuffer"));
	WeatherData.Bind(Initializer.ParameterMap, TEXT("WeatherDataBuffer"));
	MaxSnow.Bind(Initializer.ParameterMap, TEXT("MaxSnowBuffer"));
}

void FComputeShaderDeclaration::ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

void FComputeShaderDeclaration::SetParameters(FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputSurfaceUAV, FUnorderedAccessViewRHIRef SimulationCellsUAV, FUnorderedAccessViewRHIRef TemperatureDataUAV, FUnorderedAccessViewRHIRef MaxSnowUAV)
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

	if (OutputSurface.IsBound())
		RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), OutputSurfaceUAV);
	if (SimulationCells.IsBound())
		RHICmdList.SetUAVParameter(ComputeShaderRHI, SimulationCells.GetBaseIndex(), SimulationCellsUAV);
	if (WeatherData.IsBound())
		RHICmdList.SetUAVParameter(ComputeShaderRHI, WeatherData.GetBaseIndex(), TemperatureDataUAV);
	if (MaxSnow.IsBound())
		RHICmdList.SetUAVParameter(ComputeShaderRHI, MaxSnow.GetBaseIndex(), MaxSnowUAV);
}

void FComputeShaderDeclaration::SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderConstantParameters& ConstantParameters, FComputeShaderVariableParameters& VariableParameters)
{
	FComputeShaderConstantParametersRef ConstantParametersBuffer;
	FComputeShaderVariableParametersRef VariableParametersBuffer;

	ConstantParametersBuffer = FComputeShaderConstantParametersRef::CreateUniformBufferImmediate(ConstantParameters, UniformBuffer_MultiFrame);
	VariableParametersBuffer = FComputeShaderVariableParametersRef::CreateUniformBufferImmediate(VariableParameters, UniformBuffer_MultiFrame);

	SetUniformBufferParameter(RHICmdList, GetComputeShader(), GetUniformBufferParameter<FComputeShaderConstantParameters>(), ConstantParametersBuffer);
	SetUniformBufferParameter(RHICmdList, GetComputeShader(), GetUniformBufferParameter<FComputeShaderVariableParameters>(), VariableParametersBuffer);
}

/* Unbinds buffers that will be used elsewhere */
void FComputeShaderDeclaration::UnbindBuffers(FRHICommandList& RHICmdList)
{
	FComputeShaderRHIParamRef ComputeShaderRHI = GetComputeShader();

	if (OutputSurface.IsBound())
		RHICmdList.SetUAVParameter(ComputeShaderRHI, OutputSurface.GetBaseIndex(), FUnorderedAccessViewRHIRef());
}

// This is what will instantiate the shader into the engine from the engine/Shaders folder
//                      ShaderType                    ShaderFileName                Shader function name       Type
IMPLEMENT_SHADER_TYPE(, FComputeShaderDeclaration, TEXT("DegreeDaySnowSimulationComputeShader"), TEXT("MainComputeShader"), SF_Compute);

// This is required for the plugin to build
IMPLEMENT_MODULE(FDefaultModuleImpl, ComputeShader)