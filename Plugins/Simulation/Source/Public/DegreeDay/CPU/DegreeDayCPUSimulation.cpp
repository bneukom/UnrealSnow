// Fill out your copyright notice in the Description page of Project Settings.

#include "Simulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayCPUSimulation.h"
#include "SnowSimulationActor.h"
#include "Util/TextureUtil.h"
#include "Util/MathUtil.h"
#include "LandscapeComponent.h"


// @TODO Use Fearings stability method for small scale snow?
// @TODO what is the time step of Premozes simulation?
// @TODO Test Solar Radiations values from the paper

FString UDegreeDayCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day CPU"));
}

// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UDegreeDayCPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps)
{
	MaxSnow = 0;

	auto ClimateDataArray = SimulationActor->ClimateDataComponent->CreateRawClimateDataResourceArray(SimulationActor->StartTime, SimulationActor->EndTime);
	
	// Simulation
	for (auto& Cell : Cells)
	{
		const FVector& CellCentroid = Cell.Centroid;

		
		auto ClimateData = (*ClimateDataArray)[CurrentSimulationStep];
			
		float Altitude = CellCentroid.Z - SimulationActor->ClimateDataComponent->GetMeasurementAltitude(); // Altitude in cm
		float Decay = -0.5f * Altitude / (100 * 100);

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
				const float MeltFactor = TAir < TMeltB ? (TAir - TMeltA) * (TAir - TMeltA) / (TMeltB - TMeltA) : (TAir - TMeltA);
			
				const float M = c_m * MeltFactor; // l/C° * C° = l

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

		float f = Slope < 15 ? 0 : Slope / 65;
		float a3 = 50;
		float we = FMath::Max(0.0f, Cell.SnowWaterEquivalent * (1 - f) * (1 + a3 * Cell.Curvature));

		Cell.InterpolatedSnowWaterEquivalent = we;

		auto AreaSquareMeters = Cell.Area / (100 * 100);
		MaxSnow = FMath::Max(Cell.InterpolatedSnowWaterEquivalent / AreaSquareMeters, MaxSnow);
	}

	if (CaptureDebugInformation)
	{
		// Fill debug array
		for (auto& Cell : Cells)
		{
			/*
			float SnowMM = Cell.InterpolatedSWE / (Cell.Area / (100 * 100));
			DebugInformation.Add(FDebugCellInformation(SnowMM);
			*/
		}
	}
}

void UDegreeDayCPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, UWorld* World)
{
	CellsDimensionX = SimulationActor->CellsDimensionX;
	CellsDimensionY = SimulationActor->CellsDimensionY;

	// Create cells
	float ResolutionX = SimulationActor->OverallResolutionX;
	float ResolutionY = SimulationActor->OverallResolutionY;
	int CellSize = SimulationActor->CellSize;
	CellsDimensionX = SimulationActor->CellsDimensionX;
	CellsDimensionY = SimulationActor->CellsDimensionY;
	int32 NumCells = SimulationActor->NumCells;

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
			
			FSimulationCell Cell(Index, P0, P1, P2, P3, Normal, Area, AreaXY, Centroid, Altitude, Aspect, Inclination, Latitude, SnowWaterEquivalent);

			Cells.Add(Cell);
			Index++;
		}
	}

	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	auto Scale = SimulationActor->LandscapeScale;
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;

	// Calculate curvature as described in "Quantitative Analysis of Land Surface Topography" by Zevenbergen and Thorne.
	for (int32 CellIndexY = 0; CellIndexY < CellsDimensionY; ++CellIndexY)
	{
		for (int32 CellIndexX = 0; CellIndexX < CellsDimensionX; ++CellIndexX)
		{
			auto& Current = Cells[CellIndexX + CellsDimensionX * CellIndexY];

			Current.Neighbours[0] = GetCellChecked(CellIndexX, CellIndexY - 1);			// N
			Current.Neighbours[1] = GetCellChecked(CellIndexX + 1, CellIndexY - 1);		// NE
			Current.Neighbours[2] = GetCellChecked(CellIndexX + 1, CellIndexY);			// E
			Current.Neighbours[3] = GetCellChecked(CellIndexX + 1, CellIndexY + 1);		// SE

			Current.Neighbours[4] = GetCellChecked(CellIndexX, CellIndexY + 1);			// S
			Current.Neighbours[5] = GetCellChecked(CellIndexX - 1, CellIndexY + 1); 	// SW
			Current.Neighbours[6] = GetCellChecked(CellIndexX - 1, CellIndexY);			// W
			Current.Neighbours[7] = GetCellChecked(CellIndexX - 1, CellIndexY - 1);		// NW

			if (Current.AllNeighboursSet()) {

				float Z1 = Current.Neighbours[1]->Altitude / 100; // NW
				float Z2 = Current.Neighbours[0]->Altitude / 100; // N
				float Z3 = Current.Neighbours[7]->Altitude / 100; // NE
				float Z4 = Current.Neighbours[2]->Altitude / 100; // W
				float Z5 = Current.Altitude / 100;
				float Z6 = Current.Neighbours[6]->Altitude / 100; // E
				float Z7 = Current.Neighbours[3]->Altitude / 100; // SW	
				float Z8 = Current.Neighbours[4]->Altitude / 100; // S
				float Z9 = Current.Neighbours[5]->Altitude / 100; // SE

				float D = ((Z4 + Z6) / 2 - Z5) / (L * L);
				float E = ((Z2 + Z8) / 2 - Z5) / (L * L);
				Current.Curvature = 2 * (D + E);
			}
		}
	}

}

UTexture* UDegreeDayCPUSimulation::GetSnowMapTexture()
{
	// @TODO what about garbage collection and concurrency when creating this texture?
	// @TODO always create new texture too slow?

	// Create new textures
	SnowMapTexture = UTexture2D::CreateTransient(CellsDimensionX, CellsDimensionY, EPixelFormat::PF_B8G8R8A8);

	SnowMapTexture->UpdateResource();
	SnowMapTextureData.Empty(Cells.Num());

	// Update snow map texture
	for (int32 Y = 0; Y < CellsDimensionY; ++Y)
	{
		for (int32 X = 0; X < CellsDimensionX; ++X)
		{
			auto& Cell = Cells[Y * CellsDimensionX + X];

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

float UDegreeDayCPUSimulation::GetMaxSnow()
{
	return MaxSnow;
}

void UDegreeDayCPUSimulation::RenderDebug(UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType)
{

}

