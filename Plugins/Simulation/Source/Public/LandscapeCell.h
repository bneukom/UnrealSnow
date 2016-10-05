#pragma once

struct FLandscapeCell
{
	const FVector P1;

	const FVector P2;

	const FVector P3;

	const FVector P4;

	const FVector Normal;

	const int Index;

	/** Area in cm^2. */
	const float Area;

	/** Area of the cell projected onto the XY plane in cm^2. */
	const float AreaXY;

	/** Midpoint of the cell. */
	const FVector Centroid;

	/** The altitude (in cm) of the cell's mid point. */
	const float Altitude;

	/** The compass direction this cell faces. */
	const float Aspect;

	/** The slope (in radians) of this cell. */
	const float Inclination;

	/** The latitude of the center of this cell. */
	const float Latitude;

	/** Initial snow water equivalent of this cell.*/
	const float InitialWaterEquivalent;

	/** The curvature (second derivative) of the terrain for this cell. */
	float Curvature = 0.0f;



	FLandscapeCell() : Index(0), P1(FVector::ZeroVector), P2(FVector::ZeroVector), P3(FVector::ZeroVector), P4(FVector::ZeroVector),
		Normal(FVector::ZeroVector), Area(0), AreaXY(0), Centroid(FVector::ZeroVector), Altitude(0), Aspect(0), Inclination(0), Latitude(0), InitialWaterEquivalent(0) {}

	FLandscapeCell(
		int Index, FVector& p1, FVector& p2, FVector& p3, FVector& p4, FVector& Normal,
		float Area, float AreaXY, FVector Centroid, float Altitude, float Aspect, float Inclination, float Latitude, float InitialWaterEquivalent) :
		Index(Index), P1(p1), P2(p2), P3(p3), P4(p4), Normal(Normal),
		Area(Area), AreaXY(AreaXY),
		Centroid(Centroid),
		Altitude(Altitude),
		Aspect(Aspect),
		Inclination(Inclination),
		Latitude(Latitude),
		InitialWaterEquivalent(InitialWaterEquivalent)
	{
	}
};