// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "SimulationHUD.h"


void ASimulationHUD::DrawHUD()
{
	if (SimulationActor)
	{
		const auto Location = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
		auto Temperature = SimulationActor->Data->GetTemperatureData(SimulationActor->CurrentSimulationTime, SimulationActor->CurrentSimulationTime + FTimespan(24, 0, 0), FVector2D(Location.X, Location.Y), ETimespan::TicksPerDay);
		auto Precipitation = SimulationActor->Data->GetPrecipitationAt(SimulationActor->CurrentSimulationTime, SimulationActor->CurrentSimulationTime + FTimespan(24, 0, 0), FVector2D(Location.X, Location.Y), ETimespan::TicksPerDay);

		DrawText(SimulationActor->CurrentSimulationTime.ToString() + " Temperature: " + FString::SanitizeFloat(Temperature.Average) + "C" + " Precipitation: " + FString::FromInt(Precipitation) + "mm", FLinearColor::White, 5, 5, GEngine->GetLargeFont());
	}
}

void ASimulationHUD::BeginPlay()
{
	auto World = GetWorld();
	for (TActorIterator<ASnowSimulationActor> ActorItr(GetWorld(), ASnowSimulationActor::StaticClass()); ActorItr; ++ActorItr)
	{
		ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(*ActorItr);

		if (Simulation)
		{
			SimulationActor = Simulation;
			break;
		}
	}
}
