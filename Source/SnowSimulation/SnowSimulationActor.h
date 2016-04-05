// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "SnowSimulationActor.generated.h"

// @TODO use unreal debug define
#define SIMULATION_DEBUG 1

USTRUCT()
struct SNOWSIMULATION_API FSimulationCell
{
	GENERATED_BODY()

	UPROPERTY()
	FVector P1;

	UPROPERTY()
	FVector P2;

	UPROPERTY()
	FVector P3;

	UPROPERTY()
	FVector P4;

	UPROPERTY()
	FVector Normal;

	/** Area in m^3. */
	UPROPERTY()
	float Area;

	FSimulationCell() : P1(FVector::ZeroVector), P2(FVector::ZeroVector), P3(FVector::ZeroVector), P4(FVector::ZeroVector), Normal(FVector::ZeroVector) {}

	FSimulationCell(FVector& p1, FVector& p2, FVector& p3, FVector& p4, FVector& normal) : P1(p1), P2(p2), P3(p3), P4(p4), Normal(normal) {}
	
};

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
	/** Temperature decay in degrees per 100 meters of height. */
	float TemperatureDecay = -0.6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY()
	TArray<FSimulationCell> LandscapeCells;

	// Sets default values for this actor's properties
	ASnowSimulationActor();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	// Called after a property has changed
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/*
	* Creates the cells for the simulation.
	*/
	void CreateCells();

	/*
	* Sorts the cells by their z values.
	*/
	void SortCells();
	
};
