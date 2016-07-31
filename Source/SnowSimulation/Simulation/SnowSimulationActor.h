// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Landscape.h"
#include "SnowSimulation.h"
#include "GameFramework/Actor.h"
#include "GenericPlatformFile.h"
#include "Data/SimulationWeatherDataProviderBase.h"
#include "SimulationBase.h"
#include "SnowSimulationActor.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(SimulationLog, Log, All);

UENUM(BlueprintType)
enum class EDebugVisualizationType : uint8
{
	Nothing 		UMETA(DisplayName = "Nothing"),
	SnowHeight		UMETA(DisplayName = "Snow Height (mm)"),
	SWE 			UMETA(DisplayName = "Snow Water Equivalent (l)"),
	Position 		UMETA(DisplayName = "Position"),
	Altitude 		UMETA(DisplayName = "Altitude (cm)"),
	Index 			UMETA(DisplayName = "Cell Index"),
	Area 			UMETA(DisplayName = "Area (m^2)"),
	Curvature		UMETA(DisplayName = "Curvature"),
};


UCLASS()
class SNOWSIMULATION_API ASnowSimulationActor : public AActor
{
	GENERATED_BODY()
	
public:	
	//@TODO make cell creation algorithm independent of section size
	/** Size of one cell (in vertices) of the simulation, should be divisible by the quad section size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	int CellSize = 9;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The step with which the simulation runs. */
	float TimeStepHours = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Simulation")
	/** The current date of the simulation. */
	FDateTime CurrentSimulationTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Simulation start time. */
	FDateTime StartTime = FDateTime(2015, 10, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Simulation end time. */
	FDateTime EndTime = FDateTime(2016, 9, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Unit vector which points north. */
	FVector North = { 1,0,0 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The Longitude in degrees of the top left vertex of the top left cell (Northwest). */
	float Longitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The Longitude in degrees of the top left vertex of the top left cell (Northwest). */
	float Latitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The simulation used. */
	USimulationBase* Simulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Time in seconds until the next step of the simulation is executed. */
	float SleepTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** What should be visualized.  */
	EDebugVisualizationType DebugVisualizationType = EDebugVisualizationType::Nothing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** Render the simulation grid over the landscape. */
	bool RenderGrid = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** If true, writes the snow map to the path specified in Snow Map Path. */
	bool WriteDebugTextures = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	int CellDebugInfoDisplayDistance = 15000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	/** The path the snow map gets written when Write Snow Map is set to true. */
	FString DebugTexturePath = "c:\\temp";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	/** Number of simulation cells per dimension. */
	int32 CellsDimension;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	/** Landscape scale. */
	FVector LandscapeScale;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	/** Overall landscape resolution. */
	float OverallResolution;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	/** Overall landscape resolution. */
	int32 LandscapeSizeQuads;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	/** Total number of simulation cells. */
	int32 NumCells;

	/** The landscape of the world. */
	ALandscape* Landscape;

	/** Weather data provider for the simulation. */
	USimulationWeatherDataProviderBase* WeatherDataComponent;

	/** Default constructor. */
	ASnowSimulationActor();

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;
	
	/** Called every frame */
	virtual void Tick( float DeltaSeconds ) override;

#if WITH_EDITOR
	// Called after a property has changed
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	/** Initializes the simulation. */
	void Initialize();

	/** Returns the current simulation time. */
	FDateTime GetCurrentSimulationTime() const { return CurrentSimulationTime; }

private:
	/** Current simulation step time passed. */
	float CurrentStepTime;

	/** Minimum and maximum snow water equivalent (SWE) of the landscape. */
	float MinSWE, MaxSWE;

	/** 
	* Updates the material with data from the simulation.
	*/
	void UpdateMaterialTexture();

	/** 
	* Renders appropriate debug information.
	*/
	void DoRenderGrid();

};
