#pragma once

#include "WorldClimDataAssets.h"
#include "SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include "WorldClimWeatherDataProvider.generated.h"


/**
* Weather data provider which provides data from www.worldclim.org downscaled to hourly data as described in "Utility of daily vs. monthly large-scale climate data: an
* intercomparison of two statistical downscaling methods".
*/
UCLASS(Blueprintable, BlueprintType)
class SIMULATIONDATA_API UWorldClimWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<UMonthlyWorldClimDataAsset*> MonthlyData;

	virtual TResourceArray<FClimateData>* CreateRawClimateDataResourceArray(FDateTime StartTime, FDateTime EndTime) override final;

	virtual void Initialize(FDateTime StartTime, FDateTime EndTime) override final;
};
