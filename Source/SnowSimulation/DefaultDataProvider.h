#pragma once

#include "SimulationDataProviderBase.h"
#include "Array.h"
#include "DefaultDataProvider.generated.h"

/**
* Base class for all data provides for the simulation.
*/
USTRUCT(Blueprintable)
struct FMonthlyPrecipitation
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	/** Average precipitation amount in mm. */
	float AveragePrecipitation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Precipitation")
	/** Average number of days per month recorded with precipitation. */
	float AverageNumberOfDays;

	FMonthlyPrecipitation() : AveragePrecipitation(0), AverageNumberOfDays(0) {}

	FMonthlyPrecipitation(float AveragePrecipitation, float AverageNumberOfDays) : AveragePrecipitation(AveragePrecipitation), AverageNumberOfDays(AverageNumberOfDays) {}
};

/**
* Base class for all data provides for the simulation.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDefaultSimulationDataProvider : public USimulationDataProviderBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Data")
	/** Monthly temperatures. Month 0 meaning January, 1 February and so on. These temperatures are considered to be from base altitude. */
	TArray<FTemperature> MonthlyTemperatures;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation Data")
	/** Monthly precipitation values. Month 0 meaning January, 1 February and so on.*/
	TArray<FMonthlyPrecipitation> MonthlyPrecipitation;

	UDefaultSimulationDataProvider() 
	{
		// Temperature and precipitation values for Chur (CH)(http://www.weatherbase.com/weather/weather.php3?s=590618&cityname=Chur-Switzerland)
		MonthlyTemperatures.Add(FTemperature(-3,	4.7,	0.4));	// January
		MonthlyTemperatures.Add(FTemperature(-2.4,	6.4,	1.5));	// February
		MonthlyTemperatures.Add(FTemperature(1.3,	11.2,	5.7));	// March
		MonthlyTemperatures.Add(FTemperature(4.2,	15.1,	9.4));	// April
		MonthlyTemperatures.Add(FTemperature(8.6,	19.9,	14));	// May
		MonthlyTemperatures.Add(FTemperature(11.5,	22.7,	16.9)); // June
		MonthlyTemperatures.Add(FTemperature(13.4,	24.8,	18.9)); // July
		MonthlyTemperatures.Add(FTemperature(13.3,	24,		18.2)); // August
		MonthlyTemperatures.Add(FTemperature(10,	20,		14.6)); // September
		MonthlyTemperatures.Add(FTemperature(6.2,	16,		10.6)); // October
		MonthlyTemperatures.Add(FTemperature(1.4,	9.4,	4.9));	// November
		MonthlyTemperatures.Add(FTemperature(-1.7,	5.3,	1.5));	// December

		MonthlyPrecipitation.Add(FMonthlyPrecipitation(50.7,	7.3));	// January
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(47.2,	6.6));	// February
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(54.7,	8.1));	// March
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(49.2,	7.5));	// April
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(71.2,	9.9));	// May
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(92.5,	11.2)); // June
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(108.9,	11));	// July
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(112.3,	11.2)); // August
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(81.3,	8.4));	// September
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(56,		7));	// October
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(69.7,	8.5));	// November
		MonthlyPrecipitation.Add(FMonthlyPrecipitation(54.9,	7.9));	// December
	}

	virtual FTemperature GetTemperatureData(const FDateTime& Time, const FTimespan& Timespan, const FDateTime& Resolution, const FVector2D& Position) override final;

	virtual float GetPrecipitationAt(const FDateTime& Time, const FTimespan& Timespan, const FDateTime& Resolution, const FVector2D& Position) override final;

	virtual float GetVegetationDensityAt(const FVector& Position) override final;
};
