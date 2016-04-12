#include "SnowSimulation.h"
#include "UnrealMathUtility.h"
#include "SnowSimulationActor.h"
//#include "RuntimeLandscapeDataAccess.h"
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

#if SIMULATION_DEBUG
	if (Simulation) {
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Simulation type used: " + Simulation->GetSimulationName());
	}
#endif

	if (Simulation) {
		Simulation->Initialize(Cells, Data);
	}
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// @TODO update SimulationCells according to landscape and LOD

	// @TODO implement custom rendering for better performance (DrawPrimitiveUP)
	if (RenderGrid) {
		for (FSimulationCell Cell : Cells)
		{
			FVector Normal(Cell.Normal);
			Normal.Normalize();

			FVector zOffset(0, 0, GRID_Z_OFFSET);

			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P2 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P3 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);

			DrawDebugLine(GetWorld(), Cell.Centroid + zOffset, Cell.Centroid + zOffset + (Normal * NORMAL_SCALING), FColor(255, 0, 0), false, -1, 0, 0.0f);
		}
	}

}

void ASnowSimulationActor::CreateCells()
{
	// Remove old elements
	Cells.Empty();

	if (GetWorld())
	{
#if SIMULATION_DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Create Cells");
#endif
		ULevel* Level = GetWorld()->PersistentLevel;

		for (int32 iActor = 0; iActor < Level->Actors.Num(); iActor++)
		{
			// Landscape
			ALandscape* Landscape = Cast<ALandscape>(Level->Actors[iActor]);


			if (Landscape)
			{
				auto& LandscapeComponents = Landscape->LandscapeComponents;
				const int32 WorldSize = FMath::Sqrt(LandscapeComponents.Num() * Landscape->ComponentSizeQuads * Landscape->ComponentSizeQuads);

#if SIMULATION_DEBUG
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num components: " + FString::FromInt(LandscapeComponents.Num()));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num subsections: " + FString::FromInt(Landscape->NumSubsections));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num SubsectionSizeQuads: " + FString::FromInt(Landscape->SubsectionSizeQuads));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num ComponentSizeQuads: " + FString::FromInt(Landscape->ComponentSizeQuads));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num Vertices/Side: " + FString::FromInt(WorldSize));
#endif
				/*
				// @TODO only store vertices needed later
				// Get World Vertices
				TArray<FVector> CellWorldVertices;
				FRuntimeLandscapeDataCache DataInterfaceCache(0);
				for (int32 y = 0; y <= WorldSize - CellSize; y += CellSize)
				{
					int WorldX = 0; 
					for (auto Component : LandscapeComponents)
					{
						FRuntimeLandscapeComponentDataInterface LandscapeData(Component, DataInterfaceCache);

						for (int x = WorldX % Component->ComponentSizeQuads; x <= Component->ComponentSizeQuads - CellSize; WorldX += CellSize, x += CellSize)
						{
							CellWorldVertices.Add(LandscapeData.GetWorldVertex(WorldX % Component->ComponentSizeQuads, y % Component->ComponentSizeQuads));
						}
					}
				}

				for (int32 y = 0; y <= WorldSize - CellSize; y += CellSize)
				{

					for (int32 x = 0; x <= WorldSize - CellSize; x += CellSize)
					{
						FVector P0 = CellWorldVertices[y * WorldSize + x];
						FVector P1 = CellWorldVertices[y * WorldSize + (x + CellSize)];
						FVector P2 = CellWorldVertices[(y + CellSize) * WorldSize + x];
						FVector P3 = CellWorldVertices[(y + CellSize) * WorldSize + (x + CellSize)];

						FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
						FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

						float Altitude = Centroid.Z;
						float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

						FSimulationCell Cell(P0, P1, P2, P3, Normal, Area, Centroid, Altitude);

						Cells.Add(Cell);
					}
				}
				

#if SIMULATION_DEBUG
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num Vertices: " + FString::FromInt(CellWorldVertices.Num()));
#endif
				*/

				//FRuntimeLandscapeDataCache DataInterfaceCache(0);
				for (auto Component : LandscapeComponents)
				{
					FLandscapeComponentDataInterface LandscapeData(Component);
					FLandscapeComponentDataInterface LandscapeData2(Component);
					//FRuntimeLandscapeComponentDataInterface LandscapeData(Component, DataInterfaceCache);

					for (int32 y = 0; y <= Component->ComponentSizeQuads - CellSize; y += CellSize)
					{
						for (int32 x = 0; x <= Component->ComponentSizeQuads - CellSize; x += CellSize)
						{
							FVector P0 = LandscapeData.GetWorldVertex(x, y);
							FVector P1 = LandscapeData2.GetWorldVertex(x + CellSize, y);
							FVector P2 = LandscapeData.GetWorldVertex(x, y + CellSize);
							FVector P3 = LandscapeData.GetWorldVertex(x + CellSize, y + CellSize);

							FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
							FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

							float Altitude = Centroid.Z;
							float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

							FSimulationCell Cell(P0, P1, P2, P3, Normal, Area, Centroid, Altitude);

							Cells.Add(Cell);
						}
					}
				}
			}
		}

#if SIMULATION_DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num cells: " + FString::FromInt(Cells.Num()));
#endif

	}

}

#if WITH_EDITOR
void ASnowSimulationActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//Get the name of the property that was changed  
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(ASnowSimulationActor, CellSize))) {
		CreateCells();
	}

#if SIMULATION_DEBUG
	if (PropertyChangedEvent.Property) {
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "Property has changed: " + PropertyChangedEvent.Property->GetName());
	}
#endif

}
#endif

