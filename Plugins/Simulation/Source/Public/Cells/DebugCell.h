#pragma once


struct FDebugCell
{
	float SnowMM;
	float Curvature;
	
	const float Altitude;
	const FVector P1;
	const FVector P2;
	const FVector P3;
	const FVector P4;
	const FVector Centroid;
	const FVector Normal;


	FDebugCell(FVector P1, FVector P2, FVector P3, FVector P4, FVector Centroid, FVector Normal, float Altitude) : 
		P1(P1), P2(P2), P3(P3), P4(P4), Centroid(Centroid), Normal(Normal), Altitude(Altitude), SnowMM(0.0f), Curvature(0.0f) {}
};