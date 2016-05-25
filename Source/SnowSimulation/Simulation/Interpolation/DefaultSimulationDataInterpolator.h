#pragma once

#include "SimulationDataInterpolatorBase.h"
#include "Array.h"
#include "DefaultSimulationDataInterpolator.generated.h"

/**
* Base class for all data provides for the simulation.
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDefaultSimulationDataInterpolator : public USimulationDataInterpolatorBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Temperature Decay per 100m of altitude. */
	float TemperatureDecay = -0.6;

	virtual FTemperature InterpolateTemperatureByAltitude(FTemperature& BaseTemperatur, const FVector& Position) override final;

};
