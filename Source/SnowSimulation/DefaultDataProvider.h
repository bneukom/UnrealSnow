#pragma once

#include "SimulationDataProviderBase.h"
#include "Array.h"
#include "DefaultDataProvider.generated.h"


/**
* Temperature data.
*/
USTRUCT(Blueprintable)
struct FTemperature 
{
	GENERATED_USTRUCT_BODY()

	/** Minimum Temperature in degree Celsius*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Minimum;

	/** Maximum Temperature in degree Celsius*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Maximum;

	/** Mean Temperature in degree Celsius*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Mean;

	/** Temperature variance in degree Celsius*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature")
	float Variance;

	FTemperature(float minimum, float maximum, float mean, float variance) : Minimum(minimum), Maximum(maximum), Mean(mean), Variance(variance) {}

	FTemperature() : Minimum(0), Maximum(0), Mean(0), Variance(0) {}

};

/**
* Base class for all data provides for the simulation.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDefaultSimulationDataProvider : public USimulationDataProviderBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Temperature decay in degrees per 100 meters of height. */
	float TemperatureDecay = -0.6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Monthly temperatures. Month 0 meaning January, 1 February and so on. */
	TArray<FTemperature> MonthlyTemperatures;

	UDefaultSimulationDataProvider() 
	{
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
		MonthlyTemperatures.Add(FTemperature(10, 20, 15, 4));
	}
};
