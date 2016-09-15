// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Simulation/DegreeDay/DegreeDaySimulation.h"
#include "DegreeDayCPUSimulation.generated.h"


struct SNOWSIMULATION_API FSimulationCell
{
	const FVector P1;

	const FVector P2;

	const FVector P3;

	const FVector P4;

	const FVector Normal;

	const int Index;

	/** Eight neighborhood starting from north. */
	TArray<FSimulationCell*> Neighbours;

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

	/** Snow water equivalent (SWE) as the mass of water stored in liters. */
	float SnowWaterEquivalent;

	/** Snow water equivalent (SWE) after interpolation according to Blöschl. */
	float InterpolatedSnowWaterEquivalent = 0;

	/** The albedo of the snow [0-1.0]. */
	float SnowAlbedo = 0;

	/** The days since the last snow has fallen on this cell. */
	float DaysSinceLastSnowfall = 0;

	/** The curvature (second derivative) of the terrain at the given cell. */
	float Curvature = 0.0f;

	/** Returns true if all neighbours have been set (non null). */
	bool AllNeighboursSet() const {
		for (auto& Neighbour : Neighbours) {
			if (Neighbour == nullptr) return false;
		}
		return true;
	}

	/** Returns the altitude of the cells midpoint including the snow accumulated on the surface in cm. */
	float GetAltitudeWithSnow() const {
		return Altitude + GetSnowHeight() * 10;
	}

	/** Returns the snow amount in mm (or liters/m^2). */
	float GetSnowHeight() const {
		return SnowWaterEquivalent / (Area / (100 * 100));
	}

	FSimulationCell() : Index(0), P1(FVector::ZeroVector), P2(FVector::ZeroVector), P3(FVector::ZeroVector), P4(FVector::ZeroVector),
		Normal(FVector::ZeroVector), Area(0), AreaXY(0), Centroid(FVector::ZeroVector), Altitude(0), Aspect(0), Inclination(0), Latitude(0), SnowWaterEquivalent(0) {}

	FSimulationCell(
		int Index, FVector& p1, FVector& p2, FVector& p3, FVector& p4, FVector& Normal,
		float Area, float AreaXY, FVector Centroid, float Altitude, float Aspect, float Inclination, float Latitude, float SWE) :
		Index(Index), P1(p1), P2(p2), P3(p3), P4(p4), Normal(Normal),
		Area(Area), AreaXY(AreaXY),
		Centroid(Centroid),
		Altitude(Altitude),
		Aspect(Aspect),
		Inclination(Inclination),
		Latitude(Latitude)
	{
		Neighbours.Init(nullptr, 8);
	}
};

/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain". 
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDegreeDayCPUSimulation : public UDegreeDaySimulation
{
	GENERATED_BODY()
private:
	/** The cells this simulation uses. */
	TArray<FSimulationCell> Cells;

	/** The snow mask used by the landscape material. */
	UTexture2D* SnowMapTexture;

	/** Color buffer for the snow mask texture. */
	TArray<FColor> SnowMapTextureData;

	/** The maximum snow amount (mm) of the current time step. */
	float MaxSnow;

	/** Number of cells in x direction. */
	int32 CellsDimensionX;

	/** Number of cells in x direction. */
	int32 CellsDimensionY;

	/**
	* Calculates the solar radiation as described in Swifts "Algorithm for Solar Radiation on Mountain Slopes".
	*
	* @param I		The inclination of the slope in radians.
	* @param A		The aspect (compass direction) that the slope faces in radians.
	* @param L0		The latitude of the slope in radians.
	*
	* @return
	*/
	float SolarRadiationIndex(float I, float A, float L0, float J)
	{
		float L1 = FMath::Asin(FMath::Cos(I) * FMath::Sin(L0) + FMath::Sin(I) * FMath::Cos(L0) * FMath::Cos(A));
		float D1 = FMath::Cos(I) * FMath::Cos(L0) - FMath::Sin(I) * FMath::Sin(L0) * FMath::Cos(A);
		float L2 = FMath::Atan((FMath::Sin(I) * FMath::Sin(A)) / (FMath::Cos(I) * FMath::Cos(L0) - FMath::Sin(I) * FMath::Sin(L0) * FMath::Cos(A)));

		float D = 0.007 - 0.4067 * FMath::Cos((J + 10) * 0.0172);
		float E = 1.0 - 0.0167 * FMath::Cos((J - 3) * 0.0172);

		const float R0 = 1.95;
		float R1 = 60 * R0 / (E * E);
		// float R1 = (PI / 3) * R0 / (E * E);

		float T;

		T = Func2(L1, D);
		float T7 = T - L2;
		float T6 = -T - L2;
		T = Func2(L0, D);
		float T1 = T;
		float T0 = -T;
		float T3 = FMath::Min(T7, T1);
		float T2 = FMath::Max(T6, T0);

		float T4 = T2 * (12 / PI);
		float T5 = T3 * (12 / PI);

		//float R4 = Func3(L2, L1, T3, T2, R1, D); // Figure1
		if (T3 < T2) // Figure2
		{
			T2 = T3 = 0;
		}

		T6 = T6 + PI * 2;

		float R4;
		if (T6 < T1)
		{
			float T8 = T6;
			float T9 = T1;
			R4 = Func3(L2, L1, T3, T2, R1, D) + Func3(L2, L1, T9, T8, R1, D);
		} 
		else
		{
			T7 = T7 - PI * 2;

			if (T7 > T0)
			{
				float T8 = T0;
				float T9 = T0;
				R4 = Func3(L2, L1, T3, T2, R1, D) + Func3(L2, L1, T9, T8, R1, D);
			}
			else
			{
				R4 = Func3(L2, L1, T3, T2, R1, D);
			}
		}

		float R3 = Func3(0.0, L0, T1, T0, R1, D);

		return R4 / R3;
	}

	// @TODO check for invalid latitudes (90°)
	float Func2(float L, float D) // sunrise/sunset
	{
		return FMath::Acos(FMath::Clamp(-FMath::Tan(L) * FMath::Tan(D), -1.0f, 1.0f));
	}

	float Func3(float V, float W, float X, float Y, float R1, float D) // radiation
	{
		return R1 * (FMath::Sin(D) * FMath::Sin(W) * (X - Y) * (12 / PI) + 
			FMath::Cos(D) * FMath::Cos(W) * (FMath::Sin(X + V) - FMath::Sin(Y + V)) * (12 / PI));
	}

	/**
	* Returns the cell at the given x and y position or a nullptr if the indices are out of bounds.
	*
	* @param X
	* @param Y
	* @return the cell at the given x and y position or a nullptr if the indices are out of bounds.
	*/
	FSimulationCell* GetCellChecked(int X, int Y)
	{
		return GetCellChecked(X + Y * CellsDimensionX);
	}

	/**
	* Returns the cell at the given index or nullptr if the index is out of bounds.
	*
	* @param Index the index of the cell
	* @return the cell at the given index or nullptr if the index is out of bounds
	*/
	FSimulationCell* GetCellChecked(int Index)
	{
		return (Index >= 0 && Index < Cells.Num()) ? &Cells[Index] : nullptr;
	}

public:
	virtual FString GetSimulationName() override final;

	virtual void Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps) override final;

	virtual void Initialize(ASnowSimulationActor* SimulationActor, UWorld* World) override final;

	virtual void RenderDebug(UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType) override;

	virtual UTexture* GetSnowMapTexture() override final;

	virtual TArray<FColor> GetSnowMapTextureData() override final;

	virtual float GetMaxSnow() override final;
};


