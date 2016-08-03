#pragma once

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

// This buffer should contain variables that never, or rarely change
BEGIN_UNIFORM_BUFFER_STRUCT(FComputeShaderConstantParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, TotalSimulationHours)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, ClimateDataDimension)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CellsDimension)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, ThreadGroupCountX)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, ThreadGroupCountY)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, TSnowA)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, TSnowB)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, TMeltA)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, TMeltB)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, k_e)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(float, k_m)
END_UNIFORM_BUFFER_STRUCT(FComputeShaderConstantParameters)

// This buffer is for variables that change very often (each frame for example)
BEGIN_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters, )
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, CurrentSimulationStep)
DECLARE_UNIFORM_BUFFER_STRUCT_MEMBER(int, DayOfYear)
END_UNIFORM_BUFFER_STRUCT(FComputeShaderVariableParameters)

typedef TUniformBufferRef<FComputeShaderConstantParameters> FComputeShaderConstantParametersRef;
typedef TUniformBufferRef<FComputeShaderVariableParameters> FComputeShaderVariableParametersRef;

/**
* This class is what encapsulates the shader in the engine.
* It is the main bridge between the HLSL located in the engine directory and the engine itself.
*/
class FComputeShaderDeclaration : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FComputeShaderDeclaration, Global);

public:

	FComputeShaderDeclaration() {}

	explicit FComputeShaderDeclaration(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5); }

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment);

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParams = FGlobalShader::Serialize(Ar);

		Ar << OutputSurface;
		Ar << SimulationCells;
		Ar << WeatherData;
		Ar << SnowMap;
		Ar << MaxSnow;

		return bShaderHasOutdatedParams;
	}

	// Binds our runtime surface to the shader using an UAV.
	void SetParameters(
		FRHICommandList& RHICmdList, FUnorderedAccessViewRHIRef OutputSurfaceUAV, 
		FUnorderedAccessViewRHIRef SimulationCellsUAV, FUnorderedAccessViewRHIRef TemperatureDataUAV, 
		FUnorderedAccessViewRHIRef SnowMapUAV, FUnorderedAccessViewRHIRef MaxSnowUAV);

	// This function is required to bind our constant / uniform buffers to the shader.
	void SetUniformBuffers(FRHICommandList& RHICmdList, FComputeShaderConstantParameters& ConstantParameters, FComputeShaderVariableParameters& VariableParameters);
	// This is used to clean up the buffer binds after each invocation to let them be changed and used elsewhere if needed.
	void UnbindBuffers(FRHICommandList& RHICmdList);

private:
	// This is the actual output resource that we will bind to the compute shader
	FShaderResourceParameter OutputSurface;

	// Input resource which contains the cells for the simulation. 
	FShaderResourceParameter SimulationCells;

	// Temperature input data for the simulation.
	FShaderResourceParameter WeatherData;

	// Resulting snow map which is passed to the fragment shader.
	FShaderResourceParameter SnowMap;

	// Maximum snow result from the simulation.
	FShaderResourceParameter MaxSnow;
};
