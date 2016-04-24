// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SnowSimulation.h"
#include "GameFramework/Actor.h"
#include "GenericPlatformFile.h"
#include "SimulationDataProviderBase.h"
#include "SimulationBase.h"
#include "DefaultDataProvider.h"
#include "SimulationDataInterpolatorBase.h"
#include "SnowSimulationActor.generated.h"

UCLASS()
class SNOWSIMULATION_API ASnowSimulationActor : public AActor
{
	GENERATED_BODY()
	
public:	

	//@TODO make cell creation algorithm independent of section size
	/** Size of one cell of the simulation, should be divisible by the quad section size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	int CellSize = 9;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Simulation start time. */
	FDateTime StartTime = FDateTime(2015, 1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Simulation end time. */
	FDateTime EndTime = FDateTime(2015, 3, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Unit vector which points north. */
	FVector North = { 1,0,0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Data input for the simulation. */
	USimulationDataProviderBase* Data;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Interpolator for the data for the simulation. */
	USimulationDataInterpolatorBase* Interpolator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The simulation used. */
	USimulationBase* Simulation;

	TArray<FSimulationCell> Cells;

	ASnowSimulationActor();

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;
	
	/** Called every frame */
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	// Called after a property has changed
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	// @TODO CreateCells in SimulationBase.h?
	/** Removes old cells and creates the cells for the simulation. */
	void CreateCells();

private:

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

	/**
	* Computes the array index from the two dimensional grid indices X and Y.
	*
	* @param X the X cell index
	* @param Y the Y cell index
	* @return the array index from the two dimensional grid indices X and Y.
	*/
	int ToArrayIndex(int X, int Y) 
	{
		abort();
	}
};
