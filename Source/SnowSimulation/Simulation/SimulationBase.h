// Fill out your copyright notice in the Description page of Project Settings.
 
#pragma once

#include "Array.h"
#include "DateTime.h"
#include "Data/SimulationWeatherDataProviderBase.h"
#include "CellDebugInformation.h"
#include "SimulationBase.generated.h"

// Forward declarations
class ASnowSimulationActor;
enum class EDebugVisualizationType : uint8;
struct FSimulationCell;

/**
* Base class for the snow distribution simulation.
*/
UCLASS(abstract)
class SNOWSIMULATION_API USimulationBase : public UObject
{
	GENERATED_BODY()

protected:
	/** Number of cells in x direction. */
	int32 CellsDimensionX;

	/** Number of cells in y direction. */
	int32 CellsDimensionY;

	/** Whether debug information should be captured during the simulation. */
	bool CaptureDebugInformation;
public:

	/** Starts capturing debug information. */
	void startCaptureDebugInformation() { CaptureDebugInformation = true; }

	/** Stops capturing debug information. */
	void stopCaptureDebugInformation() { CaptureDebugInformation = false; }

	/**
	* Returns the name of the simulation.
	*/
	virtual FString GetSimulationName() PURE_VIRTUAL(USimulationBase::GetSimulationName, return TEXT(""););

	/**
	* Initializes the simulation.
	*/
	virtual void Initialize(ASnowSimulationActor* SimulationActor, UWorld* World) PURE_VIRTUAL(USimulationBase::Initialize, return;);

	/**
	* Runs the simulation on the given cells until the given end time is reached.
	* @param SimulationActor	the actor
	* @param TimeStep		Time step of the simulation in hours
	*/
	virtual void Simulate(ASnowSimulationActor* SimulationActor, int32 Time, int32 Timesteps) PURE_VIRTUAL(USimulationBase::Simulate, ;);

	/** Renders debug information of the simulation every tick. */
	virtual void RenderDebug(UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType VisualizationType) PURE_VIRTUAL(USimulationBase::RenderDebug, ;);

	/** Returns the maximum snow amount of any cell in mm. */
	virtual float GetMaxSnow() PURE_VIRTUAL(USimulationBase::GetMaxSnow, return 0.0f;);

	/** Returns the texture which contains the snow amount coded as gray scale values. */
	virtual UTexture* GetSnowMapTexture() PURE_VIRTUAL(USimulationBase::GetSnowMapTexture, return nullptr;);

	/** Returns the snow map texture data array. */
	virtual TArray<FColor> GetSnowMapTextureData() PURE_VIRTUAL(USimulationBase::GetSnowMapTextureData, return TArray<FColor>(););

	/** Returns debug information for the cells. Only gets called after #startCaptureDebugInformation(). */
	virtual TArray<FDebugCellInformation> GetCellDebugInformation() PURE_VIRTUAL(USimulationBase::GetCellDebugInformation, return TArray<FDebugCellInformation>(););


};




