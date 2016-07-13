// Fill out your copyright notice in the Description page of Project Settings.


#include "SnowSimulation.h"
#include "DegreeDayGPUSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Util/MathUtil.h"
#include "SnowSimulation/Simulation/Interpolation/SimulationDataInterpolatorBase.h"

FString UDegreeDayGPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day GPU"));
}

void UDegreeDayGPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime, int32 TimeStepHours)
{

}

void UDegreeDayGPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data)
{


}



