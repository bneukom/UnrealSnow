// Fill out your copyright notice in the Description page of Project Settings.


#include "SnowSimulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayCPUSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "Util/TextureUtil.h"
#include "SnowSimulation/Util/MathUtil.h"


// @TODO Use Fearings stability method for small scale snow?
// @TODO what is the time step of Premozes simulation?
// @TODO Test Solar Radiations values from the paper

FString UDegreeDayCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day CPU"));
}

// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UDegreeDayCPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep)
{
	MaxSnow = 0;

	// Simulation
	for (auto& Cell : Cells)
	{
		const FVector& CellCentroid = Cell.Centroid;

		auto ClimateData = SimulationActor->ClimateDataComponent->GetInterpolatedClimateData(SimulationActor->CurrentSimulationTime, FVector2D(CellCentroid.X, CellCentroid.Y));
			
		float Altitude = CellCentroid.Z; // Altitude in cm
		float Decay = -0.9f * Altitude / (100 * 100);

		const float TAir = ClimateData.Temperature + Decay; // degree Celsius

		const float Precipitation = ClimateData.Precipitation; // l/m^2 or mm
			
		// @TODO use AreaXY because very steep slopes with big areas would receive too much snow
		const float AreaSquareMeters = Cell.AreaXY / (100 * 100); // m^2

		// Apply precipitation
		if (Precipitation > 0)
		{
			Cell.DaysSinceLastSnowfall = 0;

			// New snow/rainfall
			const bool Rain = TAir > TSnowB;

			if (TAir > TSnowB)
			{
				Cell.SnowAlbedo = 0.4; // New rain drops the albedo to 0.4
			}
			else 
			{
				// Variable lapse rate as described in "A variable lapse rate snowline model for the Remarkables, Central Otago, New Zealand"
				float SnowRate = FMath::Clamp(1 - (TAir - TSnowA) / (TSnowB - TSnowA), 0.0f, 1.0f);

				Cell.SnowWaterEquivalent += (Precipitation * AreaSquareMeters * SnowRate); // l/m^2 * m^2 = l
				Cell.SnowAlbedo = 0.8; // New snow sets the albedo to 0.8
			}
		}

		// Apply melt
		if (Cell.SnowWaterEquivalent > 0)
		{
			if (Cell.DaysSinceLastSnowfall >= 0) {
				// @TODO is time T the degree-days or the time since the last snowfall?
				Cell.SnowAlbedo = 0.4 * (1 + FMath::Exp(-k_e * Cell.DaysSinceLastSnowfall)); 
			}

			// Temperature higher than melt threshold and cell contains snow
			if (TAir > TMeltA)
			{
				const float DayNormalization = 1.0f / 24.0f; // day 

				// @TODO radiation index at nighttime? How about newer simulations?
				// @TODO Blöschl (???) used different radiation values during night

				// Radiation Index
				const float R_i = SolarRadiationIndex(Cell.Inclination, Cell.Aspect, Cell.Latitude, SimulationActor->CurrentSimulationTime.GetDayOfYear()); // 1

				// Melt factor
				const float VegetationDensity = 0;
				const float k_v = FMath::Exp(-4 * VegetationDensity); // 1
				const float c_m = k_m * k_v * R_i *  (1 - Cell.SnowAlbedo) * DayNormalization * AreaSquareMeters; // l/m^2/C°/day * day * m^2 = l/m^2 * 1/day * day * m^2 = l/C°
				const float M = c_m *  FMath::Clamp((TAir - TMeltA) / (TMeltB - TMeltA), 0.0f, 1.0f); // l/C° * C° = l

				// Apply melt
				Cell.SnowWaterEquivalent -= M; 
				Cell.SnowWaterEquivalent = FMath::Max(0.0f, Cell.SnowWaterEquivalent);
			}
		}

		Cell.DaysSinceLastSnowfall += 1.0f / 24.0f;
	}

	// Interpolation according to Blöschls "Distributed Snowmelt Simulations in an Alpine Catchment"
	for (auto& Cell : Cells)
	{
		float Slope = FMath::RadiansToDegrees(Cell.Inclination);

		float f = Slope < 15 ? 0 : Slope / 80;
		float a3 = 50;
		float we = FMath::Max(0.0f, Cell.SnowWaterEquivalent * (1 - f) * (1 + a3 * Cell.Curvature));

		Cell.InterpolatedSnowWaterEquivalent = we;

		auto AreaSquareMeters = Cell.Area / (100 * 100);
		MaxSnow = FMath::Max(Cell.InterpolatedSnowWaterEquivalent / AreaSquareMeters, MaxSnow);
	}
}

void UDegreeDayCPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, UWorld* World)
{
	CellsDimension = SimulationActor->CellsDimension;

	float LandscapeSizeQuads = SimulationActor->LandscapeSizeQuads;
	int CellSize = SimulationActor->CellSize;
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
			FSimulationCell Cell(Index, P0, P1, P2, P3, Normal, Area, AreaXY, Centroid, Altitude, Aspect, Inclination, Latitude);

			Cells.Add(Cell);
			Index++;
		}
	}

	// Set neighbors
	for (int32 CellIndex = 0; CellIndex < NumCells; ++CellIndex)
	{
		auto& Current = Cells[CellIndex];

		Current.Neighbours[0] = GetCellChecked(CellIndex - CellsDimension);			// N
		Current.Neighbours[1] = GetCellChecked(CellIndex - CellsDimension + 1);		// NE
		Current.Neighbours[2] = GetCellChecked(CellIndex + 1);						// E
		Current.Neighbours[3] = GetCellChecked(CellIndex + CellsDimension + 1);		// SE

		Current.Neighbours[4] = GetCellChecked(CellIndex + CellsDimension);			// S
		Current.Neighbours[5] = GetCellChecked(CellIndex + CellsDimension - 1); 	// SW
		Current.Neighbours[6] = GetCellChecked(CellIndex - 1);						// W
		Current.Neighbours[7] = GetCellChecked(CellIndex - CellsDimension - 1);		// NW
	}


	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	auto Scale = SimulationActor->LandscapeScale;
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;

	// Calculate curvature as described in "Quantitative Analysis of Land Surface Topography" by Zevenbergen and Thorne.
	for (auto& Cell : Cells)
	{
		if (Cell.AllNeighboursSet()) {
			float Z1 = Cell.Neighbours[1]->Altitude / 100; // NW
			float Z2 = Cell.Neighbours[0]->Altitude / 100; // N
			float Z3 = Cell.Neighbours[7]->Altitude / 100; // NE
			float Z4 = Cell.Neighbours[2]->Altitude / 100; // W
			float Z5 = Cell.Altitude / 100;
			float Z6 = Cell.Neighbours[6]->Altitude / 100; // E
			float Z7 = Cell.Neighbours[3]->Altitude / 100; // SW	
			float Z8 = Cell.Neighbours[4]->Altitude / 100; // S
			float Z9 = Cell.Neighbours[5]->Altitude / 100; // SE

			float D = ((Z4 + Z6) / 2 - Z5) / (L * L);
			float E = ((Z2 + Z8) / 2 - Z5) / (L * L);
			Cell.Curvature = 2 * (D + E);
		}
	}
}

UTexture2D* UDegreeDayCPUSimulation::GetSnowMapTexture()
{
	// @TODO what about garbage collection and concurrency when creating this texture?
	// @TODO always create new texture too slow?

	// Create new textures
	SnowMapTexture = UTexture2D::CreateTransient(CellsDimension, CellsDimension, EPixelFormat::PF_B8G8R8A8);

	SnowMapTexture->UpdateResource();
	SnowMapTextureData.Empty(Cells.Num());

	// Update snow map texture
	for (int32 Y = 0; Y < CellsDimension; ++Y)
	{
		for (int32 X = 0; X < CellsDimension; ++X)
		{
			auto& Cell = Cells[Y * CellsDimension + X];

			// Snow map texture
			float AreaSquareMeters = Cell.Area / (100 * 100);
			float SnowMM = Cell.InterpolatedSnowWaterEquivalent / AreaSquareMeters;
			float Gray = SnowMM / GetMaxSnow() * 255;
			uint8 GrayInt = static_cast<uint8>(Gray);
			SnowMapTextureData.Add(FColor(GrayInt, GrayInt, GrayInt));
		}
	}

	// Update material
	FRenderCommandFence UpdateTextureFence;

	UpdateTextureFence.BeginFence();

	UpdateTexture(SnowMapTexture, SnowMapTextureData);

	UpdateTextureFence.Wait();

	return SnowMapTexture;
}

TArray<FColor> UDegreeDayCPUSimulation::GetSnowMapTextureData()
{
	return SnowMapTextureData;
}

void UDegreeDayCPUSimulation::RenderDebug(UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType)
{
	// Draw SWE normal
	if (DebugVisualizationType == EDebugVisualizationType::SnowHeight)
	{
		for (auto& Cell : Cells)
		{
			if (Cell.SnowWaterEquivalent > 0)
			{
				FVector Normal(Cell.Normal);
				Normal.Normalize();

				// @TODO get exact position using the height map
				FVector zOffset(0, 0, 50);

				// Height of the snow for this cell
				auto AreaSquareMeters = Cell.Area / (100 * 100);
				auto SWE = Cell.SnowWaterEquivalent / AreaSquareMeters; // l/m^2 or mm
																		//DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + zOffset + (Normal * SWE * 10), FColor(255, 0, 0), false, -1, 0, 0.0f);
				DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + FVector(0, 0, SWE / 10) + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);
			}
		}
	}
	const auto Location = World->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	auto PlayerController = World->GetFirstPlayerController();
	auto Pawn = PlayerController->GetPawn();

	// Render snow water equivalent
	int Index = 0;
	for (auto& Cell : Cells)
	{
		auto Offset = Cell.Normal;
		Offset.Normalize();

		// @TODO get position from heightmap
		Offset *= 10;

		if (FVector::Dist(Cell.Centroid + Offset, Location) < CellDebugInfoDisplayDistance)
		{
			FCollisionQueryParams TraceParams(FName(TEXT("Trace SWE")), true);
			TraceParams.bTraceComplex = true;
			TraceParams.AddIgnoredActor(Pawn);

			//Re-initialize hit info
			FHitResult HitOut(ForceInit);

			World->LineTraceSingleByChannel(HitOut, Location, Cell.P1 + Offset, ECC_WorldStatic, TraceParams);

			auto Hit = HitOut.GetActor();

			//Hit any Actor?
			if (Hit == NULL)
			{
				switch (DebugVisualizationType)
				{
				case EDebugVisualizationType::SWE:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.SnowWaterEquivalent)), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::SnowHeight:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.GetSnowHeight())) + " mm", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Position:
					DrawDebugString(World, Cell.Centroid, "(" + FString::FromInt(static_cast<int>(Cell.Centroid.X / 100)) + "/" + FString::FromInt(static_cast<int>(Cell.Centroid.Y / 100)) + ")", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Altitude:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.Altitude / 100)) + "m", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Index:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(Index), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Curvature:
					DrawDebugString(World, Cell.Centroid, FString::SanitizeFloat(Cell.Curvature), nullptr, FColor::Purple, 0, true);
					break;
				default:
					break;
				}
			}
		}

		Index++;
	}
}

