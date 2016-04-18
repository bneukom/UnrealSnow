// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationDataInterpolator.generated.h"

/**
* Base class for all data providers for the simulation.
*/
UCLASS(BlueprintType)
class SNOWSIMULATION_API USimulationDataInterpolator : public UObject
{
	GENERATED_BODY()

public:
	/**
	* Returns the temperature data at base elevation at the given day of the year and position (2D).
	*/
	virtual FTemperature GetInterpolatedTemperatureData(FTemperature& BaseTemperatur, const FVector& Position) PURE_VIRTUAL(USimulationDataInterpolator::GetDailyTemperatureData, return FTemperature(););

	/**
	* Returns the temperature data at base elevation at the given day of the year and position (2D).
	*/
	virtual int GetDaysSinceLastSnow(const FVector& Position) PURE_VIRTUAL(USimulationDataInterpolator::GetDailyTemperatureData, return 0);

};