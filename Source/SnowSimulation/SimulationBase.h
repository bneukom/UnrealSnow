// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SimulationBase.generated.h"

/**
 * Base class for the snow distribution simulation.
 */
UCLASS(abstract)
class SNOWSIMULATION_API USimulationBase : public UObject
{
	GENERATED_BODY()
public:
	virtual FString ToString() = 0;
};
