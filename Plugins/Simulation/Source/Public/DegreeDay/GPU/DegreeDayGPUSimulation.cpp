
#include "Simulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayGPUSimulation.h"
#include "SnowSimulationActor.h"
#include "Util/MathUtil.h"
#include "LandscapeComponent.h"

FString UDegreeDayGPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day GPU"));
}

void UDegreeDayGPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps)
{
	SimulationComputeShader->ExecuteComputeShader(CurrentSimulationStep, Timesteps, SimulationActor->CurrentSimulationTime.GetHour(), CaptureDebugInformation, CellDebugInformation);
	SimulationPixelShader->ExecutePixelShader(RenderTarget);
}

void UDegreeDayGPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, UWorld* World)
{
	// Create shader
	SimulationComputeShader = new FSimulationComputeShader(World->Scene->GetFeatureLevel());
	SimulationPixelShader = new FSimulationPixelShader(World->Scene->GetFeatureLevel());

	// Create cells
	float ResolutionX = SimulationActor->OverallResolutionX;
	float ResolutionY = SimulationActor->OverallResolutionY;
	int CellSize = SimulationActor->CellSize;
	CellsDimensionX = SimulationActor->CellsDimensionX;
	CellsDimensionY = SimulationActor->CellsDimensionY;
	int32 NumCells = SimulationActor->NumCells;

	CellDebugInformation.SetNumUninitialized(CellsDimensionX * CellsDimensionY);

	TArray<FVector> CellWorldVertices;
	CellWorldVertices.SetNumUninitialized(ResolutionX * ResolutionY);

	auto& LandscapeComponents = SimulationActor->Landscape->LandscapeComponents;
	for (auto Component : LandscapeComponents)
	{
		// @TODO use runtime compatible version
		FLandscapeComponentDataInterface LandscapeData(Component);
		for (int32 Y = 0; Y < Component->ComponentSizeQuads; Y++) // not +1 because the vertices are stored twice (first and last)
		{
			for (int32 X = 0; X < Component->ComponentSizeQuads; X++) // not +1 because the vertices are stored twice (first and last)
			{
				CellWorldVertices[Component->SectionBaseX + X + ResolutionX * Y + Component->SectionBaseY * ResolutionX] = LandscapeData.GetWorldVertex(X, Y);
			}
		}
	}

	// Create Cells
	TResourceArray<FComputeShaderSimulationCell> Cells;
	float MaxSnow = 0.0f;
	float MinAltitude = 1e6;
	float MaxAltitude = 0;

	int Index = 0;
	for (int32 Y = 0; Y < CellsDimensionY; Y++)
	{
		for (int32 X = 0; X < CellsDimensionX; X++)
		{
			auto VertexX = X * CellSize;
			auto VertexY = Y * CellSize;
			FVector P0 = CellWorldVertices[VertexY * ResolutionX + VertexX];
			FVector P1 = CellWorldVertices[VertexY * ResolutionX + (VertexX + CellSize)];
			FVector P2 = CellWorldVertices[(VertexY + CellSize) * ResolutionX + VertexX];
			FVector P3 = CellWorldVertices[(VertexY + CellSize) * ResolutionX + (VertexX + CellSize)];

			FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
			FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

			float Altitude = Centroid.Z;
			MinAltitude = FMath::Min(MinAltitude, Altitude);
			MaxAltitude = FMath::Max(MaxAltitude, Altitude);
			float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

			float AreaXY = FMath::Abs(FVector2D::CrossProduct(FVector2D(P0 - P3), FVector2D(P1 - P3)) / 2
				+ FVector2D::CrossProduct(FVector2D(P2 - P3), FVector2D(P0 - P3)) / 2);

			FVector P0toP3 = P3 - P0;
			FVector P0toP3ProjXY = FVector(P0toP3.X, P0toP3.Y, 0);
			float Inclination = IsAlmostZero(P0toP3.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(P0toP3, P0toP3ProjXY) / (P0toP3.Size() * P0toP3ProjXY.Size())));

			// @TODO assume constant for the moment, later handle in input data
			const float Latitude = FMath::DegreesToRadians(47);

			// @TODO what is the aspect of the XY plane?
			FVector NormalProjXY = FVector(Normal.X, Normal.Y, 0);
			float Aspect = IsAlmostZero(NormalProjXY.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(SimulationActor->North, NormalProjXY) / NormalProjXY.Size()));
			
			// Initial conditions
			float SnowWaterEquivalent = 0.0f;
			if (Altitude / 100.0f > 3300.0f)
			{
				auto AreaSquareMeters = Area / (100 * 100);
				float we = (2.5 + Altitude / 100 * 0.001) * AreaSquareMeters;

				SnowWaterEquivalent = we;

				MaxSnow = FMath::Max(SnowWaterEquivalent / AreaSquareMeters, MaxSnow);
			}
			
			FComputeShaderSimulationCell Cell(Aspect, Inclination, Altitude, Latitude, Area, AreaXY, SnowWaterEquivalent);
			Cells.Add(Cell);

			//FDebugCellInformation DebugCell();
			//CellDebugInformation.Add(DebugCell);

			Index++;
		}
	}


	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	auto Scale = SimulationActor->LandscapeScale;
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;

	for (int32 CellIndexY = 0; CellIndexY < CellsDimensionY; ++CellIndexY)
	{
		for (int32 CellIndexX = 0; CellIndexX < CellsDimensionX; ++CellIndexX)
		{
			FComputeShaderSimulationCell& Cell = Cells[CellIndexX + CellsDimensionX * CellIndexY];
			int Neighbours[8];

			Neighbours[0] = GetCellChecked(CellIndexX, CellIndexY - 1);						// N
			Neighbours[1] = GetCellChecked(CellIndexX + 1, CellIndexY - 1);					// NE
			Neighbours[2] = GetCellChecked(CellIndexX + 1, CellIndexY);						// E
			Neighbours[3] = GetCellChecked(CellIndexX + 1, CellIndexY + 1);					// SE

			Neighbours[4] = GetCellChecked(CellIndexX,	CellIndexY + 1);					// S
			Neighbours[5] = GetCellChecked(CellIndexX - 1, CellIndexY + 1); 				// SW
			Neighbours[6] = GetCellChecked(CellIndexX - 1, CellIndexY);						// W
			Neighbours[7] = GetCellChecked(CellIndexX - 1, CellIndexY - 1);					// NW

			if (Neighbours[0] == -1 || Neighbours[1] == -1 || Neighbours[2] == -1 || Neighbours[3] == -1
				|| Neighbours[4] == -1 || Neighbours[5] == -1 || Neighbours[6] == -1 || Neighbours[7] == -1) continue;

			float Z1 = Cells[Neighbours[1]].Altitude / 100; // NW
			float Z2 = Cells[Neighbours[0]].Altitude / 100; // N
			float Z3 = Cells[Neighbours[7]].Altitude / 100; // NE
			float Z4 = Cells[Neighbours[2]].Altitude / 100; // W
			float Z5 = Cell.Altitude / 100;
			float Z6 = Cells[Neighbours[6]].Altitude / 100; // E
			float Z7 = Cells[Neighbours[3]].Altitude / 100; // SW	
			float Z8 = Cells[Neighbours[4]].Altitude / 100; // S
			float Z9 = Cells[Neighbours[5]].Altitude / 100; // SE

			float D = ((Z4 + Z6) / 2 - Z5) / (L * L);
			float E = ((Z2 + Z8) / 2 - Z5) / (L * L);
			Cell.Curvature = 2 * (D + E);
		}
	}

	// Initialize render target
 	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(CellsDimensionX, CellsDimensionY);
	
	// Initialize shader
	auto ClimateData = SimulationActor->ClimateDataComponent->CreateRawClimateDataResourceArray(SimulationActor->StartTime, SimulationActor->EndTime);
	auto SimulationTimeSpan = SimulationActor->EndTime - SimulationActor->StartTime;
	int32 TotalHours = static_cast<int32>(SimulationTimeSpan.GetTotalHours());

	SimulationComputeShader->Initialize(Cells, *ClimateData, k_e, k_m, TMeltA, TMeltB, TSnowA, TSnowB, TotalHours, CellsDimensionX, CellsDimensionY, SimulationActor->ClimateDataComponent->GetMeasurementAltitude(), MaxSnow);
	SimulationPixelShader->Initialize(SimulationComputeShader->GetSnowBuffer(), SimulationComputeShader->GetMaxSnowBuffer(), CellsDimensionX, CellsDimensionY);
}

UTexture* UDegreeDayGPUSimulation::GetSnowMapTexture()
{
	UTexture* RenderTargetTexture = Cast<UTexture>(RenderTarget);
	return  RenderTargetTexture;
}

TArray<FColor> UDegreeDayGPUSimulation::GetSnowMapTextureData()
{
	return TArray<FColor>();
}

float UDegreeDayGPUSimulation::GetMaxSnow()
{
	return SimulationComputeShader->GetMaxSnow();
}

