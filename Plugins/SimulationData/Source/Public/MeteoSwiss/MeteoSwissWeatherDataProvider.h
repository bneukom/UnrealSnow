#pragma once

#include "SimulationWeatherDataProviderBase.h"
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
class SIMULATIONDATA_API UMeteoSwissWeatherDataProvider : public USimulationWeatherDataProviderBase
{
	GENERATED_BODY()

private:
	TArray<FClimateData> ClimateData;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	UDataTable* TemperatureData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	UDataTable* PrecipitationData;

	/** The altitude of the measuring station in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Climate)
	float StationAltitude;

	virtual TResourceArray<FClimateData>* CreateRawClimateDataResourceArray(FDateTime StartTime, FDateTime EndTime) override final;

	virtual void Initialize(FDateTime StartTime, FDateTime EndTime) override final;

	virtual float GetMeasurementAltitude() override final;
};