#pragma once

#include "Private/WorldClimDataPrivatePCH.h"
#include "Array.h"
#include "BILData.generated.h"

UCLASS()
class UBILData : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<uint8> Data;
};
