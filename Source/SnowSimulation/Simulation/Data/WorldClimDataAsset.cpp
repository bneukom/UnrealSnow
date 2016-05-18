#include "SnowSimulation.h"
#include "WorldClimDataAsset.h"

// @TODO how to round?
// @TODO Only works for single band images
int16 UWorldClimDataAsset::GetDataAt(float Lat, float Long)
{
	// @TODO Check if Lat/Long is inside of bounds
	int32 OffsetX = FMath::FloorToInt((Long - HDR->ULXMAP) / HDR->XDIM);
	int32 OffsetY = FMath::FloorToInt((HDR->ULYMAP - Lat) / HDR->YDIM);

	int Index = HDR->NCOLS * OffsetY + OffsetX;
	return Data->Data[Index];
}
