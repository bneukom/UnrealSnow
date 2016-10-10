// Fill out your copyright notice in the Description page of Project Settings.

#include "Simulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayCPUSimulation.h"
#include "SnowSimulationActor.h"
#include "Cells/GPUSimulationCell.h"
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
void UDegreeDayCPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps, bool SaveSnowMap, bool CaptureDebugInformation, TArray<FDebugCell>& DebugCells)
{
	MaxSnow = 0;

	auto ClimateDataArray = SimulationActor->ClimateDataComponent->CreateRawClimateDataResourceArray(SimulationActor->StartTime, SimulationActor->EndTime);
	
	// Simulation
	for (auto& Cell : Cells)
	{
		const FVector& CellCentroid = Cell.Centroid;

		
		auto ClimateData = (*ClimateDataArray)[CurrentSimulationStep];
			
		float Altitude = CellCentroid.Z - SimulationActor->ClimateDataComponent->GetMeasurementAltitude(); // Altitude in cm
		
		float TemperatureLapse = -0.5f * Altitude / (100 * 100);
		float PrecipitationLapse = 0.5f * Altitude / (100 * 1000);

		const float TAir = ClimateData.Temperature + TemperatureLapse; // degree Celsius

		const float Precipitation = ClimateData.Precipitation + PrecipitationLapse; // l/m^2 or mm
			
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

void UDegreeDayCPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, const TArray<FLandscapeCell>& LandscapeCells, float InitialMaxSnow, UWorld* World)
{
	// Create Cells
	TResourceArray<FGPUSimulationCell> Cells;
	for (const FLandscapeCell& LandscapeCell : LandscapeCells)
	{
		FGPUSimulationCell Cell(LandscapeCell.Aspect, LandscapeCell.Inclination, LandscapeCell.Altitude,
			LandscapeCell.Latitude, LandscapeCell.Area, LandscapeCell.AreaXY, LandscapeCell.InitialWaterEquivalent);
		Cells.Add(Cell);
	}

	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	auto Scale = SimulationActor->LandscapeScale;
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;
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

	// Update texture
	FRenderCommandFence UpdateTextureFence;

	UpdateTextureFence.BeginFence();

	UpdateTexture(SnowMapTexture, SnowMapTextureData);

	UpdateTextureFence.Wait();

	return SnowMapTexture;
}

float UDegreeDayCPUSimulation::GetMaxSnow()
{
	return MaxSnow;
}

void UDegreeDayCPUSimulation::RenderDebug(UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType)
{

}

