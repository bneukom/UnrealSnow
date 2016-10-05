#pragma once


struct FDebugCell
{
	float SnowMM;
	FVector P1;
	FVector P2;
	FVector P3;
	FVector P4;
	FVector Centroid;
	FVector Normal;

	FDebugCell(FVector P1, FVector P2, FVector P3, FVector P4, FVector Centroid, FVector Normal) : 
		P1(P1), P2(P2), P3(P3), P4(P4), Centroid(Centroid), Normal(Normal) {}
};