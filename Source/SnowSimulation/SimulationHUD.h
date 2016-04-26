// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EngineUtils.h"
#include "SnowSimulationActor.h"
#include "SnowSimulation.h"
#include "GameFramework/HUD.h"
#include "SimulationHUD.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API ASimulationHUD : public AHUD
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** The simulation used. */
	ASnowSimulationActor* SimulationActor;


	/************************************************************************/
	/* Overriden methods                                                    */
	/************************************************************************/
	virtual void DrawHUD() override;

	virtual void BeginPlay() override;
};
