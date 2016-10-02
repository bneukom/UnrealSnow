#pragma once


struct FDebugCellInformation
{
	float SnowMM;
	FVector P1;
	FVector P2;
	FVector P3;
	FVector P4;
	FVector Centroid;

	FDebugCellInformation(float SnowMM) : SnowMM(SnowMM) {}
};