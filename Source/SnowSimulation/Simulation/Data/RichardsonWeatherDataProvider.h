#pragma once

#include "SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include "RichardsonWeatherDataProvider.generated.h"

/**
* Weather provider which implements a single site Richardson-type stochastic weather generator downscaled to hourly weather data. 
* Noise is added to provide regional scale precipitation patterns.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API URichardsonWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

public:
	virtual FTemperature GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* SnowSimulation, int64 Resolution) override final;

	virtual float GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution) override final;

};