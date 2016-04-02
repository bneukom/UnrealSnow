// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "SnowSimulationActor.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeComponent.h"
#include "Runtime/Landscape/Classes/LandscapeInfo.h"
#include "Runtime/Landscape/Classes/LandscapeProxy.h"
#include "Runtime/Landscape/Public/LandscapeDataAccess.h"

#define SIMULATION_DEBUG 0

ASnowSimulationActor::ASnowSimulationActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ASnowSimulationActor::BeginPlay()
{
	Super::BeginPlay();

	CreateCells();
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#ifdef SIMULATION_DEBUG
	DrawDebugSphere(
		GetWorld(),
		FVector(0, 0, 0),
		200,
		32,
		FColor(255, 0, 0)
		);
#endif
}

void ASnowSimulationActor::CreateCells()
{
	if (GetWorld())
	{
#ifdef SIMULATION_DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Create Cells");
#endif
		ULevel* Level = GetWorld()->PersistentLevel;

		for (int32 iActor = 0; iActor < Level->Actors.Num(); iActor++)
		{
			// Landscape
			ALandscape* Landscape = Cast<ALandscape>(Level->Actors[iActor]);

			if (Landscape)
			{
				auto LandscapeComponents = Landscape->LandscapeComponents;

				// Get all components
				for (auto Component : LandscapeComponents)
				{
					FLandscapeComponentDataInterface LandscapeData(Component);

					for (int32 y = 0; y < Component->ComponentSizeQuads; y++)
					{
						for (int32 x = 0; x < Component->ComponentSizeQuads; x++)
						{
							FVector P0 = LandscapeData.GetWorldVertex(x, y);

							FVector P1 = LandscapeData.GetWorldVertex(x + 1, y + 1);
							FVector P2 = LandscapeData.GetWorldVertex(x + 1, y);

							FVector Normal = FVector::CrossProduct(P2 - P0, P1 - P0);

							FLandscapeCell Cell(P0, P1, Normal);

							LandscapeCells.Add(Cell);
						}
					}
				}
			}
		}

#ifdef SIMULATION_DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::FromInt(LandscapeCells.Num()));

		for (int i = 0; i < 10; i++)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "[" + LandscapeCells[i].P1.ToString() + ", " + LandscapeCells[i].P2.ToString() + ", " + LandscapeCells[i].Normal.ToString() + "]");
		}
	}
#endif
}

