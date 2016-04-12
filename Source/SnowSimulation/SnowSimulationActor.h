// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "GenericPlatformFile.h"
#include "SimulationDataProviderBase.h"
#include "SimulationBase.h"
#include "DefaultDataProvider.h"
#include "SnowSimulationActor.generated.h"



// @TODO use unreal debug define
#define SIMULATION_DEBUG 1

UCLASS()
class SNOWSIMULATION_API ASnowSimulationActor : public AActor
{
	GENERATED_BODY()
	
public:	

#ifdef SIMULATION_DEBUG
	static const int GRID_Z_OFFSET = 10;
	static const int NORMAL_SCALING = 100;
#endif // SIMULATION_DEBUG

	//@TODO make cell creation algorithm independent of section size
	/** Size of one cell of the simulation, should be divisible by the quad section size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	int CellSize = 9;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Data input for the simulation. */
	USimulationDataProviderBase* Data;

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

	/*
	* Removes old cells and creates the cells for the simulation.
	*/
	void CreateCells();


};
