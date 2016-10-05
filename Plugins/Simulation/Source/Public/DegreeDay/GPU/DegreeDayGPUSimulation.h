#pragma once
// Fill out your copyright notice in the Description page of Project Settings.

#include "SimulationPixelShader.h"
#include "SimulationComputeShader.h"
#include "DegreeDay/DegreeDaySimulation.h"
#include "SimulationBase.h"
#include "DebugCell.h"
#include "LandscapeCell.h"
#include "DegreeDayGPUSimulation.generated.h"


/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain".
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SIMULATION_API UDegreeDayGPUSimulation : public UDegreeDaySimulation
{
	GENERATED_BODY()

private:
	FSimulationComputeShader* SimulationComputeShader;

	FSimulationPixelShader* SimulationPixelShader;

	UTextureRenderTarget2D* RenderTarget;

public:

	virtual FString GetSimulationName() override final;

	virtual void Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps, bool SaveSnowMap, bool CaptureDebugInformation, TArray<FDebugCell> DebugCells) override final;

	virtual void Initialize(ASnowSimulationActor* SimulationActor, const TArray<FLandscapeCell>& Cells, float InitialMaxSnow, UWorld* World) override final;

	virtual UTexture* GetSnowMapTexture() override final;

	virtual float GetMaxSnow() override final;
};

