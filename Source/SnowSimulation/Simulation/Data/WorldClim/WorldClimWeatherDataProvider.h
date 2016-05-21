#pragma once

#include "WorldClimDataAssets.h"
#include "SnowSimulation/Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include "WorldClimWeatherDataProvider.generated.h"


/**
* Weather data provider which provides data from www.worldclim.org downscaled to hourly data as described in "Utility of daily vs. monthly large-scale climate data: an
* intercomparison of two statistical downscaling methods".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UWorldClimWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()
public:
	// @TODO implement
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	//UMonthlyDataDownscaling* DataDownscaling;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<UMonthlyWorldClimDataAsset*> MonthlyData;

	virtual FTemperature GetTemperatureData(const FDateTime& Date, const FDateTime& To, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution) override final;

	virtual float GetPrecipitationAt(const FDateTime& Date, const FDateTime& To, const FVector2D& Position, int64 Resolution) override final;
};
