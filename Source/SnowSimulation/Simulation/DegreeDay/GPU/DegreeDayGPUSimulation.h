#pragma once
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationComputeShader.h"
#include "Simulation/DegreeDay/DegreeDaySimulation.h"
#include "SnowSimulation/Simulation/SimulationBase.h"
#include "DegreeDayGPUSimulation.generated.h"



/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain".
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDegreeDayGPUSimulation : public UDegreeDaySimulation
{
	GENERATED_BODY()

private:
	FSimulationComputeShader* ComputeShader;

public:

	virtual FString GetSimulationName() override final;

	virtual void Simulate(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime, int32 TimeStepHours) override final;

	virtual void Initialize(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data) override final;
};

