// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationBase.h"
#include "PremozeCPUSimulation.generated.h"

/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain". 
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(BlueprintType)
class SNOWSIMULATION_API UPremozeCPUSimulation : public USimulationBase
{
	GENERATED_BODY()


public:
	virtual FString GetSimulationName() override final;

	virtual void Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, int RunTime) override final;

	virtual void Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data) override final;
};

