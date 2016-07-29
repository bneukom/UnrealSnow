// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RHIResources.h"
#include "WeatherData.h"
#include "SimulationWeatherDataProviderBase.generated.h"

class ASnowSimulationActor;

// @TODO simplify API because most weather data sources only provide monthly or daily average values.
// @TODO use stochastic downscaling for hourly weather data
// @TODO extend ActorComponent?
/**
 * Base class for all data providers for the simulation.
 */
UCLASS(BlueprintType)
class SNOWSIMULATION_API USimulationWeatherDataProviderBase : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Initializes the data provider. */
	virtual void Initialize() PURE_VIRTUAL(USimulationWeatherDataProviderBase::Initialize, ;);

	/** Returns the climate Data at the given cell. */
	virtual FWeatherData GetInterpolatedClimateData(const FDateTime& TimeStamp, int IndexX, int IndexY) PURE_VIRTUAL(USimulationWeatherDataProviderBase::GetInterpolatedClimateData, return FWeatherData(););

	/** Returns the climate Data at the given position. */
	FWeatherData GetInterpolatedClimateData(const FDateTime& TimeStamp, const FVector2D& Position);

	/** Returns the resolution of this weather data provider. */
	virtual int32 GetResolution() PURE_VIRTUAL(USimulationWeatherDataProviderBase::GetResolution, return 0;);

	/** Returns the climate Data at the given cell. */
	virtual TResourceArray<FWeatherData>* GetRawClimateData(const FDateTime& TimeStamp) PURE_VIRTUAL(USimulationWeatherDataProviderBase::GetInterpolatedClimateData, return nullptr;);


};


