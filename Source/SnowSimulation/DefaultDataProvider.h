#pragma once

#include "SimulationDataProviderBase.h"
#include "Array.h"
#include "DefaultDataProvider.generated.h"

/**
* Base class for all data provides for the simulation.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDefaultSimulationDataProvider : public USimulationDataProviderBase
{
	GENERATED_BODY()

public:


	float DailyTemperatureVariance = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Monthly temperatures. Month 0 meaning January, 1 February and so on. These temperatures are considered to be from 0 altitude. */
	TArray<FTemperature> MonthlyTemperatures;

	UDefaultSimulationDataProvider() 
	{
		MonthlyTemperatures.Add(FTemperature(-10, 5, -2)); // January
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // February
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // March
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // April
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // May
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // June
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // July
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // August
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // September
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // October
		MonthlyTemperatures.Add(FTemperature(10, 20, 15)); // November
		MonthlyTemperatures.Add(FTemperature(-10, 10, 0)); // December
	}
	virtual FTemperature GetDailyTemperatureData(const int Day, const FVector2D& Position) override final;

	virtual float GetPrecipitationAt(const FDateTime& Time, const FVector2D& Position) override final;

	virtual float GetVegetationDensityAt(const FVector& Position) override final;
};
