// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "SnowSimulationActor.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeComponent.h"
#include "Runtime/Landscape/Classes/LandscapeInfo.h"
#include "Runtime/Landscape/Classes/LandscapeProxy.h"
#include "Runtime/Landscape/Public/LandscapeDataAccess.h"


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

	// @TODO implement custom shader for better performance
#ifdef SIMULATION_DEBUG
	for (FLandscapeCell Cell : LandscapeCells)
	{
		FVector Normal(Cell.Normal);
		Normal.Normalize();

		FVector zOffset(0, 0, GRID_Z_OFFSET);

		DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P2 + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);
		DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P3 + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);

		DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P1 + zOffset + (Normal * NORMAL_SCALING), FColor(0, 255, 0), false, -1, 0, 0.0f);
		
	}
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
#ifdef SIMULATION_DEBUG
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num components: " + FString::FromInt(LandscapeComponents.Num()));
#endif
				// Get all components
				for (auto Component : LandscapeComponents)
				{
					FLandscapeComponentDataInterface LandscapeData(Component);

					for (int32 y = 0; y <= Component->ComponentSizeQuads - CELL_SIZE; y += CELL_SIZE)
					{
						for (int32 x = 0; x <= Component->ComponentSizeQuads - CELL_SIZE; x += CELL_SIZE)
						{
							FVector P0 = LandscapeData.GetWorldVertex(x, y);
							FVector P1 = LandscapeData.GetWorldVertex(x + CELL_SIZE, y);
							FVector P2 = LandscapeData.GetWorldVertex(x, y + CELL_SIZE);
							FVector P3 = LandscapeData.GetWorldVertex(x + CELL_SIZE, y + CELL_SIZE);

							FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);

							FLandscapeCell Cell(P0, P1, P2, P3, Normal);

							LandscapeCells.Add(Cell);
						}
					}
				}
			}
		}

#ifdef SIMULATION_DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num cells: " + FString::FromInt(LandscapeCells.Num()));

	}
#endif
}

