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
private:
	TArray<FSimulationCell> SlopeThresholdCells;

public:
	/** Slope threshold for the snow deposition of the cells in degrees.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float SlopeThreshold = 60;

	/** Threshold air temperature above which all precipitation is assumed to be rain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float TSnow = 0;

	/** Threshold air temperature above which snow starts melting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float TMelt = 0;

	/** Time constant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float k_e = 0.2;

	/** Proportional constant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float k_m = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	/** Temperature decay in degrees per 100 meters of altitude. */
	float TemperatureDecay = -0.6;

	virtual FString GetSimulationName() override final;

	virtual void Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, FDateTime& StartTime, FDateTime& EndTime) override final;

	virtual void Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data) override final;

#if SIMULATION_DEBUG
	virtual void RenderDebug(TArray<FSimulationCell>& Cells) override final;
#endif 

};

