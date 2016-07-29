#pragma once

#include "ComputeShaderSimulationCell.h"
#include "Private/ComputeShaderDeclaration.h"
#include "WeatherData.h"
#include "RWStructuredBuffer.h"




/**
* This class demonstrates how to use the compute shader we have declared.
* Most importantly which RHI functions are needed to call and how to get 
* some interesting output.                                                
*/
class COMPUTESHADER_API FSimulationComputeShader
{
public:
	FSimulationComputeShader(ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FSimulationComputeShader();

	/** Initializes the simulation with the correct input data. */
	void Initialize(TResourceArray<FComputeShaderSimulationCell>& Cells, float k_e, float k_m, float TMeltA, float TMeltB, float TSnowA, float TSnowB, int32 CellsDimension, int32 WeatherDataResolution);

	// @TODO create on heap and pass pointer?

	/**
	* Run this to execute the compute shader once!
	* @param TotalElapsedTimeSeconds - We use this for simulation state 
	*/
	void ExecuteComputeShader(int TimeStep, TResourceArray<FWeatherData>* ClimateData);

	/**
	* Only execute this from the render thread.
	*/
	void ExecuteComputeShaderInternal(TResourceArray<FWeatherData>* ClimateData);

	FTexture2DRHIRef GetTexture() { return Texture; }

private:
	bool IsComputeShaderExecuting;
	bool IsUnloading;
	bool Debug;

	int32 NumCells;

	FComputeShaderConstantParameters ConstantParameters;
	FComputeShaderVariableParameters VariableParameters;
	ERHIFeatureLevel::Type FeatureLevel;

	/** Main texture */
	FTexture2DRHIRef Texture;

	/** We need a UAV if we want to be able to write to the resource*/
	FUnorderedAccessViewRHIRef TextureUAV;

	/** Cells for the simulation. */
	FRWStructuredBuffer* SimulationCellsBuffer;

	/** Temperature data for the simulation. */
	FRWStructuredBuffer* ClimateDataBuffer;
};
