#pragma once

#include "Cells/GPUSimulationCell.h"
#include "Private/ComputeShaderDeclaration.h"
#include "ClimateData.h"
#include "RWStructuredBuffer.h"
#include "Cells/DebugCell.h"

DECLARE_LOG_CATEGORY_EXTERN(SnowComputeShader, Log, All);

/**
* This class demonstrates how to use the compute shader we have declared.
* Most importantly which RHI functions are needed to call and how to get 
* some interesting output.                                                
*/
class SIMULATION_API FSimulationComputeShader
{
public:
	FSimulationComputeShader(ERHIFeatureLevel::Type ShaderFeatureLevel);
	~FSimulationComputeShader();

	/** Initializes the simulation with the correct input data. */
	void Initialize(
		TResourceArray<FGPUSimulationCell>& Cells, TResourceArray<FClimateData>& WeatherData, float k_e, float k_m, 
		float TMeltA, float TMeltB, float TSnowA, float TSnowB, int32 TotalSimulationHours, 
		int32 CellsDimensionX, int32 CellsDimensionY,  float MeasurementAltitude, float MaxSnow);

	/**
	* Run this to execute the compute shader once!
	* @param TotalElapsedTimeSeconds - We use this for simulation state 
	*/
	void ExecuteComputeShader(int CurrentTimeStep, int32 Timesteps, int HourOfDay, bool CaptureDebugInformation, TArray<FDebugCell>& DebugInformation);

	/**
	* Only execute this from the render thread.
	*/
	void ExecuteComputeShaderInternal(bool CaptudeDebugInformation, TArray<FDebugCell>& DebugInformation);

	/** 
	* Returns the maximum snow of the last execution.
	*/
	float GetMaxSnow();

	FTexture2DRHIRef GetTexture() { return Texture; }

	FRWStructuredBuffer* GetSnowBuffer() { return SnowOutputBuffer; }

	FRWStructuredBuffer* GetMaxSnowBuffer() { return MaxSnowBuffer; }
private:
	bool IsComputeShaderExecuting;
	bool IsUnloading;
	bool Debug = false;

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

	/** Maximum snow buffer. */
	FRWStructuredBuffer* MaxSnowBuffer;
	float MaxSnow;

	/** Output snow map array. */
	FRWStructuredBuffer* SnowOutputBuffer;
};
