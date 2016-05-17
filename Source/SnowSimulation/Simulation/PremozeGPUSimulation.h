
#pragma once

#include "SimulationBase.h"
#include "PremozeGPUSimulation.generated.h"

/**
* GPU implementation of the snow distribution simulation.
*/
UCLASS(BlueprintType)
class SNOWSIMULATION_API UGPUSimulation : public USimulationBase
{
	GENERATED_BODY()


public:
	virtual FString GetSimulationName() override final;
};

