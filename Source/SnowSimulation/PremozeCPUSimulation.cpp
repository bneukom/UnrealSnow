// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "PremozeCPUSimulation.h"

FString UPremozeCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Premoze CPU"));
}

void UPremozeCPUSimulation::Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, int RunTime)
{
	// 
}

void UPremozeCPUSimulation::Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data)
{
	Cells.Sort([](const FSimulationCell& A, const FSimulationCell& B) 
	{
		return A.Altitude < B.Altitude;
	});
}
