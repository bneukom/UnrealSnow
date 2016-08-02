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

private:
	int32 Resolution;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TArray<UMonthlyWorldClimDataAsset*> MonthlyData;

	virtual FClimateData GetInterpolatedClimateData(const FDateTime& TimeStamp, int IndexX, int IndexY) override final;

	virtual TResourceArray<FClimateData>* CreateRawClimateDataResourceArray() override final;

	virtual int32 GetResolution() override final;

	virtual void Initialize() override final;
};
