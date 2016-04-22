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
		Simulation->Simulate(Cells, Data, Interpolator, FDateTime(2015, 1, 1), FDateTime(2015, 12, 1));
	}
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// @TODO update SimulationCells according to landscape and LOD



#if SIMULATION_DEBUG
	if (Simulation)
	{
		Simulation->RenderDebug(Cells);
	}
#endif // SIMULATION_DEBUG
	

	// @TODO implement custom rendering for better performance (DrawPrimitiveUP)
	if (RenderGrid) {
		for (FSimulationCell& Cell : Cells)
		{
			FVector Normal(Cell.Normal);
			Normal.Normalize();

			// @TODO get exact position using the height map
			FVector zOffset(0, 0, 50);

			// Draw Cells
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P2 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P3 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P2 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P3 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);

			// Draw normal
			DrawDebugLine(GetWorld(), Cell.Centroid + zOffset, Cell.Centroid + zOffset + (Normal * 100), FColor(255, 0, 0), false, -1, 0, 0.0f);
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
			ALandscape* Landscape = Cast<ALandscape>(Level->Actors[iActor]);

			if (Landscape)
			{
				auto& LandscapeComponents = Landscape->LandscapeComponents;
				const int32 LandscapeSizeQuads = FMath::Sqrt(LandscapeComponents.Num() * (Landscape->ComponentSizeQuads) * (Landscape->ComponentSizeQuads));

#if SIMULATION_DEBUG
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num components: " + FString::FromInt(LandscapeComponents.Num()));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Num subsections: " + FString::FromInt(Landscape->NumSubsections));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "SubsectionSizeQuads: " + FString::FromInt(Landscape->SubsectionSizeQuads));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "ComponentSizeQuads: " + FString::FromInt(Landscape->ComponentSizeQuads));
				GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "LandscapeSizeQuads: " + FString::FromInt(LandscapeSizeQuads));
#endif

				// @TODO what about sections?
				// Get Vertices from all components in the landscape
				TArray<FVector> CellWorldVertices;
				CellWorldVertices.SetNumUninitialized(LandscapeSizeQuads * LandscapeSizeQuads);

				for (auto Component : LandscapeComponents)
				{
					// @TODO use runtime compatible version
					FLandscapeComponentDataInterface LandscapeData(Component);
					for (int32 Y = 0; Y < Component->ComponentSizeQuads; Y++) // not +1 because the vertices are stored twice (first and last)
					{
						for (int32 X = 0; X < Component->ComponentSizeQuads; X++) // not +1 because the vertices are stored twice (first and last)
						{
							CellWorldVertices[Component->SectionBaseX + X + LandscapeSizeQuads * Y + Component->SectionBaseY * LandscapeSizeQuads] = LandscapeData.GetWorldVertex(X, Y);
						}

					}
					// @TODO insert vertices at the very end which are currently not added because we are only iterating over Quads
				}
				
				// Create Cells
				const int32 CellsDimension = LandscapeSizeQuads / CellSize - 1; // -1 because we create cells and use 4 vertices
				const int32 NumCells = CellsDimension * CellsDimension;

				const float Latitude = FMath::DegreesToRadians(47); //  @TODO assume constant for the moment

				for (int32 Y = 0; Y < CellsDimension; Y++) 
				{
					for (int32 X = 0; X < CellsDimension; X++)
					{
						auto VertexX = X * CellSize;
						auto VertexY = Y * CellSize;
						FVector P0 = CellWorldVertices[VertexY * LandscapeSizeQuads + VertexX];
						FVector P1 = CellWorldVertices[VertexY * LandscapeSizeQuads + (VertexX + CellSize)];
						FVector P2 = CellWorldVertices[(VertexY + CellSize) * LandscapeSizeQuads + VertexX];
						FVector P3 = CellWorldVertices[(VertexY + CellSize) * LandscapeSizeQuads + (VertexX + CellSize)];

						FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
						FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

						float Altitude = Centroid.Z;
						float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

						FVector NormalProjXY = FVector(Normal.X, Normal.Y, 0);
						float Inclination = FMath::Abs(FMath::Acos(FVector::DotProduct(Normal, NormalProjXY) / (Normal.Size() * NormalProjXY.Size())));
						float Aspect = 0; // @TODO calculate aspect
						FSimulationCell Cell(P0, P1, P2, P3, Normal, Area, Centroid, Altitude, Aspect, Inclination, Latitude);

						Cells.Add(Cell);
					}
				}

				// Set neighbors
				for (int32 CellIndex = 0; CellIndex < NumCells; ++CellIndex)
				{
					auto& Current = Cells[CellIndex];

					Current.Neighbours[0] = GetCellChecked(CellIndex - CellsDimension);				// N
					if ((CellIndex + 1) % CellsDimension != 0) 
						Current.Neighbours[1] = GetCellChecked(CellIndex - CellsDimension + 1);		// NE
					if ((CellIndex + 1) % CellsDimension != 0)
						Current.Neighbours[2] = GetCellChecked(CellIndex + 1);						// E
					if ((CellIndex + 1) % CellsDimension != 0)
						Current.Neighbours[3] = GetCellChecked(CellIndex + CellsDimension + 1);		// SE
					
					Current.Neighbours[4] = GetCellChecked(CellIndex + CellsDimension);				// S
					if ((CellIndex) % CellsDimension != 0)
						Current.Neighbours[5] = GetCellChecked(CellIndex + CellsDimension - 1); 	// SW
					if ((CellIndex) % CellsDimension != 0)
						Current.Neighbours[6] = GetCellChecked(CellIndex - 1);						// N
					if ((CellIndex) % CellsDimension != 0)
						Current.Neighbours[7] = GetCellChecked(CellIndex - CellsDimension - 1);		// NW
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

