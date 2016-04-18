// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationDataProviderBase.generated.h"

/**
* Precipitation data.
*/
USTRUCT(Blueprintable)
struct FPrecipitation
{
	GENERATED_USTRUCT_BODY()
	// Amount of precipitation in liter/(m^2) = mm.
	float Amount;
};

/**
* Temperature data.
*/
USTRUCT(Blueprintable)
struct FTemperature
{
	GENERATED_USTRUCT_BODY()

	/** Minimum Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Minimum;

	/** Maximum Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Maximum;

	/** Mean Temperature in degree Celsius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Mean;

	FTemperature(float minimum, float maximum, float mean) : Minimum(minimum), Maximum(maximum), Mean(mean) {}

	FTemperature() : Minimum(0), Maximum(0), Mean(0) {}
};

/**
 * Base class for all data providers for the simulation.
 */
UCLASS(BlueprintType)
class SNOWSIMULATION_API USimulationDataProviderBase : public UObject
{
	GENERATED_BODY()

private:
	USimulationDataInterpolator* interpolator;

public:
	/** 
	* Returns the temperature data at base elevation at the given day of the year and position (2D).
	*/
	virtual FTemperature GetDailyTemperatureData(const int Day, const FVector2D& Position) PURE_VIRTUAL(USimulationDataProviderBase::GetDailyTemperatureData, return FTemperature(););

	/**
	* Returns the precipitation at base elevation at the given time and position (2D).
	*/
	virtual float GetPrecipitationAt(const FDateTime& Time, const FVector2D& Position) PURE_VIRTUAL(USimulationDataProviderBase::GetPrecipitationAt, return 0.0f;);

	/**
	* Returns the vegetation density [0-1.0] at the given position.
	*/
	virtual float GetVegetationDensityAt(const FVector& Position) PURE_VIRTUAL(USimulationDataProviderBase::GetVegetationDensityAt, return 0.0f;);


};