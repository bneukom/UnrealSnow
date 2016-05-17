#pragma once

#include "HDRData.h"
#include "BILData.h"
#include "Engine/DataAsset.h"
#include "WorldClimDataAsset.generated.h"

UCLASS(BlueprintType)
class UWorldClimDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	UHDRData* HDR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	UBILData* Data;

	/* Returns the data at the given latitude and longitude. */
	int16 GetDataAt(float Lat, float Long);
};
