// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationBase.h"
#include "CPUSimulation.generated.h"

/**
* CPU implementation of the snow distribution simulation.
*/
UCLASS(BlueprintType)
class SNOWSIMULATION_API UCPUSimulation : public USimulationBase
{
	GENERATED_BODY()

public:
	virtual FString ToString() override final;
};

