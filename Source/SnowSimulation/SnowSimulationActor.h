// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Landscape.h"
#include "SnowSimulation.h"
#include "GameFramework/Actor.h"
#include "GenericPlatformFile.h"
#include "SimulationDataProviderBase.h"
#include "SimulationBase.h"
#include "DefaultDataProvider.h"
#include "SimulationDataInterpolatorBase.h"
#include "SnowSimulationActor.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(SimulationLog, Log, All);

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
	/** The step with which the simulation runs. */
	float TimeStepHours = 1;

	// @TODO setting these does not work in the editor
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Time in seconds until the next step of the simulation is executed. */
	float SleepTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** If true, render debug information over the landscape. */
	bool RenderDebugInfo = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** If true, writes the snow map to the path specified in Snow Map Path. */
	bool WriteSnowMap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	int CellDebugInfoDisplayDistance = 15000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** The path the snow map gets written when Write Snow Map is set to true. */
	FString DebugTexturePath = "c:\\temp\\snowmap";

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
	/** Initializes the simulation. */
	void Initialize();

	/** Returns the current simulation time. */
	FDateTime GetCurrentSimulationTime() const { return CurrentSimulationTime; }
private:
	/** The landscape of the world. */
	ALandscape* Landscape;

	/** The current date of the simulation. */
	FDateTime CurrentSimulationTime;

	/** Current simulation step time passed. */
	float CurrentStepTime;

	/** The snow mask used by the landscape material. */
	UTexture2D* SnowMaskTexture;

	/** Color buffer for the snow mask texture. */
	TArray<FColor> SnowMaskTextureData;

	/** The snow mask used by the landscape material. */
	UTexture2D* SWETexture;

	/** Color buffer for the snow mask texture. */
	TArray<FColor> SWETextureData;

	/** Minimum and maximum snow water equivalent (SWE) of the landscape. */
	float MinSWE, MaxSWE;

	/** The cells used by the simulation. */
	TArray<FSimulationCell> Cells;

	/** Total number of simulation cells. */
	int32 NumCells;

	/** Number of simulation cells per dimension. */
	int32 CellsDimension;

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
		//@TODO implement
		abort();
	}

	/** 
	* Updates the material with data from the simulation.
	*/
	void UpdateMaterialTexture();

	/** 
	* Updates the texture with the given texture data.
	*/
	void UpdateTexture(UTexture2D* Texture, TArray<FColor>& TextureData);
};
