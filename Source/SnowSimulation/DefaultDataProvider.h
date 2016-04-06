#pragma once

#include "SimulationDataProviderBase.h"
#include "DefaultDataProvider.generated.h"


/**
* Base class for all data provides for the simulation.
*/
UCLASS(Blueprintable)
class SNOWSIMULATION_API UDefaultSimulationDataProvider : public USimulationDataProviderBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Temperature decay in degrees per 100 meters of height. */
	float TemperatureDecay = -0.6;

};
