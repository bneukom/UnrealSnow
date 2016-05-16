#pragma once

#include "Array.h"
#include "BILData.generated.h"

USTRUCT()
struct FHDR
{
	GENERATED_USTRUCT_BODY()

	int32 NROWS;
	int32 NCOLS;
	int32 NBANDS;
	int32 NBITS;
	int32 BANDROWBYTES;
	int32 TOTALROWBYTES;
	int32 BANDGAPBYTES;
	int32 NODATA;

	float ULXMAP;
	float ULYMAP;
	float XDIM;
	float YDIM;

	int32 MinX;
	int32 MaxX;
	int32 MinY;
	int32 MaxY;
	int32 MinValue;
	int32 MaxValue;
	int32 month;
};

UCLASS()
class UBILData : public UObject
{
	GENERATED_BODY()

public:
	FHDR HDR;
	TArray<int16> Data;
};

UCLASS()
class UWorldClimBILData : public UObject
{
	GENERATED_BODY()

public:

	TArray<int16> Data;
};

