#pragma once

#include "SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include "RichardsonWeatherDataProvider.generated.h"


struct SNOWSIMULATION_API FModelParameter
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	/** Probability of a wet day given the previous day was wet. */
	float P_WW;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	/** Probability of a wet day given the previous day was dry. */
	float P_WD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float Beta;
};

/** State of the simulation. */
enum class WeatherState : int8
{
	WET, DRY 
};

/**
* Weather provider which implements a single site Richardson-type stochastic weather generator downscaled to hourly weather data. 
* Noise is added to provide regional scale precipitation patterns.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API URichardsonWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

private:
	/** State of the simulation. */
	WeatherState State;

	/** Generated temperature data. */
	TArray<FTemperature> TemperatureData;

	/** Generated precipitation data. */
	TArray<FPrecipitation> PrecipitationData;
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	/** Initial probability of a wet day. */
	float P_WI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	/** Monthly parameter values derieved from real data. */
	TArray<FModelParameter> MonthlyParameters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FDateTime From;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	FDateTime To;

	URichardsonWeatherDataProvider();

	virtual FTemperature GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* SnowSimulation, int64 Resolution) override final;

	virtual float GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution) override final;

};