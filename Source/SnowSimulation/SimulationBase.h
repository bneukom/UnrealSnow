// Fill out your copyright notice in the Description page of Project Settings.
 
#pragma once

#include "Array.h"
#include "DateTime.h"
#include "SimulationDataProviderBase.h"
#include "SimulationDataInterpolatorBase.h"
#include "SimulationBase.generated.h"


struct SNOWSIMULATION_API FSimulationCell
{
	const FVector P1;

	const FVector P2;

	const FVector P3;

	const FVector P4;

	const FVector Normal;

	/** Eight neighborhood starting from north. */
	TArray<FSimulationCell*> Neighbours;

	/** Area in cm^3. */
	const float Area;

	/** Midpoint of the cell. */
	const FVector Centroid;

	/** The altitude of the cell's mid point. */
	const float Altitude;

	/** The compass direction this cell faces. */
	const float Aspect;

	/** The slope of this cell. */
	const float Inclination;

	/** The latitude of the center of this cell. */
	const float Latitude;

	// @TODO Create template (subclasses?) with this data, other simulations might use other data.
	/** Snow water equivalent (SWE) as the mass of water stored as liter. */
	float SnowWaterEquivalent = 0;
	
	/** The albedo of the snow [0-1.0]. */
	float SnowAlbedo = 0;

	/** The days since the last snow has fallen on this cell. */
	int DaysSinceLastSnowfall = 0;

	/** Neighbour with the steepest downward slope. */
	FSimulationCell* SteepestDownwardSlopeNeighbour = nullptr;

	float SteepestDownwardSlope = 0;

	FSimulationCell() : P1(FVector::ZeroVector), P2(FVector::ZeroVector), P3(FVector::ZeroVector), P4(FVector::ZeroVector),
		Normal(FVector::ZeroVector), Area(0), Centroid(FVector::ZeroVector), Altitude(0), Aspect(0), Inclination(0), Latitude(0) {}

	FSimulationCell(
		FVector& p1, FVector& p2, FVector& p3, FVector& p4, FVector& Normal, 
		float Area, FVector Centroid, float Altitude, float Aspect, float Inclination, float Latitude) :
		P1(p1), P2(p2), P3(p3), P4(p4), Normal(Normal), 
		Area(Area), 
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
 * Base class for the snow distribution simulation.
 */
UCLASS(abstract)
class SNOWSIMULATION_API USimulationBase : public UObject
{
	GENERATED_BODY()

public:
	
	/** Timestep of the simulation in hours. */
	UPROPERTY()
	int32 TimeStepHours = 1;

	/**
	* Returns the name of the simulation.
	*/
	virtual FString GetSimulationName() PURE_VIRTUAL(USimulationBase::GetSimulationName, return TEXT(""););

	/** 
	* Initializes the simulation.
	*/
	virtual void Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data) PURE_VIRTUAL(USimulationBase::Initialize, ;);

	/** 
	* Runs the simulation on the given cells until the given end time is reached.
	* @param Cells			cells on which the simulation runs
	* @param Data			input data used for the simulation
	* @param Interpolator	used to interpolate input data
	* @param RunTime		time to run the simulation in hours
	*/
	virtual void Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime) PURE_VIRTUAL(USimulationBase::Run, ;);

#if SIMULATION_DEBUG
	/** Renders debug information of the simulation every tick. */
	virtual void RenderDebug(TArray<FSimulationCell>& Cells) PURE_VIRTUAL(USimulationBase::RenderDebug, ;);
#endif 
};
