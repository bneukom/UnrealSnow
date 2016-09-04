#pragma once

#include "SnowSimulation/Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "Array.h"
#include "MeteoSwissWeatherDataProvider.generated.h"

USTRUCT(BlueprintType)
struct FPrecipitationData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	FPrecipitationData() : StationName(""), Precipitation(0)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Precipitation)
	FString StationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Precipitation)
	float Precipitation;
};

USTRUCT(BlueprintType)
struct FTemperatureData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

public:

	FTemperatureData() : StationName(""), Temperature(0)
	{}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Temperature)
	FString StationName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Temperature)
	float Temperature;
};

/**
* 
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UMeteoSwissWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

private:
	int32 Resolution;

	TArray<FClimateData> ClimateData;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	UDataTable* TemperatureData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	UDataTable* PrecipitationData;

	/** The altitude of the measuring station in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	float StationAltitude;

	virtual FClimateData GetInterpolatedClimateData(const FDateTime& TimeStamp, int IndexX, int IndexY) override final;

	virtual TResourceArray<FClimateData>* CreateRawClimateDataResourceArray() override final;

	virtual int32 GetResolution() override final;

	virtual void Initialize() override final;

	virtual float GetMeasurementAltitude() override final;
};