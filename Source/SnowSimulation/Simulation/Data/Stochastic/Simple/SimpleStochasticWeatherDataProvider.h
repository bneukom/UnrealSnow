#pragma once

#include "Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include <vector>
#include "SimpleStochasticWeatherDataProvider.generated.h"


struct FNoisePrecipitation
{
public:
	/** Base precipitation amount. */
	float Precipitation;

	/** Distribution of the precipitation. */
	std::vector<std::vector<float>> Noise;

	FNoisePrecipitation() : 
		Precipitation(0),
		Noise(std::vector<std::vector<float>>(0, std::vector<float>(0)))
	{}

	FNoisePrecipitation(float Precipitation, int32 Dimension) : 
		Precipitation(Precipitation), 
		Noise(std::vector<std::vector<float>>(Dimension, std::vector<float>(Dimension)))
	{}

	// FNoisePrecipitation(FNoisePrecipitation&& Other) = default;
	/*{
		Precipitation = Other.Precipitation;
		Noise = std::move(Other.Noise);
	}*/

	/*
	FNoisePrecipitation(const FNoisePrecipitation& Other)
	{
		Precipitation = Other.Precipitation;
		Noise = Other.Noise;
	}
	*/
	/** Returns the precipitation at the given cell. */
	float GetPrecipitationAt(int X, int Y)
	{
		float N = (Noise.size() == 0) ? 1 : Noise[X][Y];
		return Precipitation * N;
	}
};

/** State of the simulation. */
enum class WeatherState : int8
{
	WET, DRY 
};

/**
* Simple stochastic weather provider which generates hourly precipitation using a two state Markov chain which does not 
* change transition probabilities during the day or during seasons. Temperature follows a simple sinusoidal pattern 
* and precipitation amount follows an exponential distribution. To account for spatial variation noise is applied
* to the precipitation. The temperature and the precipitation are not correlated.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API USimpleStochasticWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

private:
	/** State of the simulation. */
	WeatherState State;

	/** Generated temperature data. */
	TArray<FTemperature> TemperatureData;

	/** Generated precipitation data. */
	TArray<FNoisePrecipitation> PrecipitationData;

	/** Temperature noise. */
	std::vector<std::vector<float>> TemperatureNoise;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	/** Noise dimension cell size in cm. Smaller dimensions result in higher noise resolution but worse performance. */
	float NoiseCellDimension = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", DisplayName = "P_I_W")
	/** Initial probability of a wet day. */
	float P_I_W = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", DisplayName = "P_WD")
	/** Probability of a wet hour given the previous hour was dry. */
	float P_WD = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", DisplayName = "P_WW")
	/** Probability of a wet hour given the previous hour was wet. */
	float P_WW = 0.6;


	USimpleStochasticWeatherDataProvider();

	virtual FTemperature GetTemperatureData(const FDateTime& From, const FDateTime& To, const FVector2D& Position, ASnowSimulationActor* SnowSimulation, int64 Resolution) override final;

	virtual float GetPrecipitationAt(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution) override final;

	virtual void Initialize();

};