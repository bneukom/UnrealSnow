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

	// @TODO Degrees or radians?
	/**
	* Calculates the solar radiation as described in Swifts "Algorithm for Solar Radiation on Mountain Slopes".
	* @param I		The inclination of the slope in degrees.
	* @param A		The aspect (compass direction) that the slope faces.
	* @param L0		The latitude of the slope.
	*
	* @return
	*/
	float SolarRadiationIndex(float I, float A, float L0, float J)
	{
		float L1 = FMath::Asin(FMath::Cos(I) * FMath::Sin(L0) * FMath::Sin(I) * FMath::Cos(L0) *FMath::Cos(A));
		float L2 = FMath::Atan((FMath::Sin(I) * FMath::Sin(A)) / (FMath::Cos(I) * FMath::Cos(L0) - FMath::Sin(I) * FMath::Sin(L0) * FMath::Cos(A)));

		float D = 0.4 - 23.3 * FMath::Cos((J + 10) * 0.986);
		float E = 1.0 - 0.0167 * FMath::Cos((J - 3) * 0.986);

		const float R0 = 2;
		float R1 = 60 * R0 / (E * E);

		float T_L1 = Func2(L1, D);
		float T_L0 = Func2(L0, D);
		float T1 = T_L1;
		float T0 = -T_L1;
		float T3 = FMath::Min(T_L1 - L2, T1);
		float T2 = FMath::Min(-T_L1 - L2, T0);

		float R4 = Func3(L2, L1, T3, T2, R1, D);
		float R3 = Func3(0.0, L0, T1, T0, R1, D);

		return R4 / R3;
	}

	// @TODO check for invalid latitudes (90°)
	float Func2(float L, float D) // sunrise/sunset
	{
		return FMath::Acos(-FMath::Tan(L) * FMath::Tan(D));
	}

	float Func3(float V, float W, float X, float Y, float R1, float D) // radiation
	{
		return R1 * (FMath::Sin(D) * FMath::Sin(W) * (X - Y) / 15 + 
			FMath::Cos(D) * FMath::Cos(W) * (FMath::Sin(X + V) - FMath::Sin(Y + V)) * PI);
	}

public:
	/** Slope threshold for the snow deposition of the cells in radians.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Simulation")
	float SlopeThreshold = PI / 3;

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
	/** Temperature decay in degrees Celsius per 100 meters of altitude. */
	float TemperatureDecay = -0.6;

	virtual FString GetSimulationName() override final;

	virtual void Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, FDateTime& StartTime, FDateTime& EndTime) override final;

	virtual void Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data) override final;

#if SIMULATION_DEBUG
	virtual void RenderDebug(TArray<FSimulationCell>& Cells) override final;
#endif 

};

