// Fill out your copyright notice in the Description page of Project Settings.
 
#pragma once

#include "Array.h"
#include "DateTime.h"
#include "SimulationDataProviderBase.h"
#include "SimulationBase.generated.h"

struct SNOWSIMULATION_API FSimulationCell
{
	const FVector P1;

	const FVector P2;

	const FVector P3;

	const FVector P4;

	const FVector Normal;

	/** Eight neighborhood starting from north. */
	const TArray<FSimulationCell*> Neighbours;

	/** Area in m^3. */
	const float Area;

	/** Midpoint of the cell. */
	const FVector Centroid;

	/** The altitude of the cell's mid point. */
	const float Altitude;

	/** Snow water equivalent (SWE) of the cell in m^3. */
	float SnowWaterEquivalent;

	FSimulationCell() : P1(FVector::ZeroVector), P2(FVector::ZeroVector), P3(FVector::ZeroVector), P4(FVector::ZeroVector),
		Normal(FVector::ZeroVector), Area(0), Centroid(FVector::ZeroVector), Altitude(0) {}

	FSimulationCell(FVector& p1, FVector& p2, FVector& p3, FVector& p4, FVector& normal, float area, FVector centroid, float altitude) :
		P1(p1), P2(p2), P3(p3), P4(p4), Normal(normal), Area(area), Centroid(centroid), Altitude(altitude) {}
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
	int32 TimeStep = 1;

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
	* @param Cells		cells on which the simulation runs
	* @param Data		input data used for the simulation
	* @param RunTime	time to run the simulation in hours
	*/
	virtual void Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, int RunTime) PURE_VIRTUAL(USimulationBase::Run, ;);
};
