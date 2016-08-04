#include "SnowSimulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayGPUSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Util/MathUtil.h"

FString UDegreeDayGPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day GPU"));
}

void UDegreeDayGPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep)
{
	SimulationComputeShader->ExecuteComputeShader(CurrentSimulationStep);
	SimulationPixelShader->ExecutePixelShader(RenderTarget);
}

void UDegreeDayGPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, UWorld* World)
{
	// Create shader
	SimulationComputeShader = new FSimulationComputeShader(World->Scene->GetFeatureLevel());
	SimulationPixelShader = new FSimulationPixelShader(World->Scene->GetFeatureLevel());

	// Create cells
	float LandscapeSizeQuads = SimulationActor->LandscapeSizeQuads;
	int CellSize = SimulationActor->CellSize;
	int32 CellsDimension = SimulationActor->CellsDimension;
	int32 NumCells = SimulationActor->NumCells;

	TArray<FVector> CellWorldVertices;
	CellWorldVertices.SetNumUninitialized(SimulationActor->LandscapeSizeQuads * SimulationActor->LandscapeSizeQuads);

	auto& LandscapeComponents = SimulationActor->Landscape->LandscapeComponents;
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
	TResourceArray<FComputeShaderSimulationCell> Cells;
	
	int Index = 0;
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
			FComputeShaderSimulationCell Cell(Aspect, Inclination, Altitude, Latitude, Area, AreaXY);

			Cells.Add(Cell);
			Index++;
		}
	}


	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	auto Scale = SimulationActor->LandscapeScale;
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;

	// Calculate curvature as described in "Quantitative Analysis of Land Surface Topography" by Zevenbergen and Thorne.
	for (int32 CellIndex = 0; CellIndex < NumCells; ++CellIndex)
	{
		FComputeShaderSimulationCell& Cell = Cells[CellIndex];
		int Neighbours[8];

		Neighbours[0] = GetCellChecked(CellIndex - CellsDimension, NumCells);			// N
		Neighbours[1] = GetCellChecked(CellIndex - CellsDimension + 1, NumCells);		// NE
		Neighbours[2] = GetCellChecked(CellIndex + 1, NumCells);						// E
		Neighbours[3] = GetCellChecked(CellIndex + CellsDimension + 1, NumCells);		// SE

		Neighbours[4] = GetCellChecked(CellIndex + CellsDimension, NumCells);			// S
		Neighbours[5] = GetCellChecked(CellIndex + CellsDimension - 1, NumCells); 	// SW
		Neighbours[6] = GetCellChecked(CellIndex - 1, NumCells);						// W
		Neighbours[7] = GetCellChecked(CellIndex - CellsDimension - 1, NumCells);		// NW

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

	// Initialize render target
 	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(CellsDimension, CellsDimension);
	
	// Initialize shader
	auto ClimateData = SimulationActor->ClimateDataComponent->CreateRawClimateDataResourceArray();
	auto SimulationTimeSpan = SimulationActor->EndTime - SimulationActor->StartTime;
	int32 TotalHours = static_cast<int32>(SimulationTimeSpan.GetTotalHours());

	SimulationComputeShader->Initialize(Cells, *ClimateData, k_e, k_m, TMeltA, TMeltB, TSnowA, TSnowB, TotalHours, CellsDimension, SimulationActor->ClimateDataComponent->GetResolution());
	SimulationPixelShader->Initialize(SimulationComputeShader->GetSnowBuffer(), SimulationComputeShader->GetMaxSnowBuffer(), CellsDimension);

	delete ClimateData;
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

