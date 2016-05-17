// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationDataInterpolatorBase.generated.h"

/**
* Base class for all data providers for the simulation.
*/
UCLASS(BlueprintType)
class SNOWSIMULATION_API USimulationDataInterpolatorBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	* Returns the interpolated temperature with the given base temperature at the given position.
	*/
	virtual FTemperature InterpolateTemperatureByAltitude(FTemperature& BaseTemperatur, const FVector& Position) PURE_VIRTUAL(USimulationDataInterpolator::GetDailyTemperatureData, return FTemperature(););

};