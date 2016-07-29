#pragma once
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "SnowSimulation/Simulation/SimulationBase.h"
#include "DegreeDaySimulation.generated.h"


/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain".
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDegreeDaySimulation : public USimulationBase
{
	GENERATED_BODY()
protected:
	/** The maximum snow amount (mm) of the current time step. */
	float MaxSnow;

public:
	/** Slope threshold for the snow deposition of the cells in degrees.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float SlopeThreshold = 45;

	/** Threshold A air temperature above which some precipitation is assumed to be rain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "TSnow A")
	float TSnowA = 0;

	/** Threshold B air temperature above which all precipitation is assumed to be rain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "TSnow B")
	float TSnowB = 2;

	/** Threshold A air temperature above which some snow starts melting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "TMelt A")
	float TMeltA = 0;

	/** Threshold B air temperature above which all snow starts melting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "TMelt B")
	float TMeltB = 2;

	/** Time constant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "k_e")
	float k_e = 0.2;

	/** Proportional constant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation", DisplayName = "k_m")
	float k_m = 1;


	virtual float GetMaxSnow() override final;
};

