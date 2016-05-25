// Fill out your copyright notice in the Description page of Project Settings.


#include "SnowSimulation.h"
#include "DegreeDayCPUSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Util/MathUtil.h"
#include "SnowSimulation/Simulation/Interpolation/SimulationDataInterpolatorBase.h"

FString UDegreeDayCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Premoze CPU"));
}

// @TODO what is the time step of Premozes simulation?
// @TODO Test Solar Radiations values from the paper

// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UDegreeDayCPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, USimulationWeatherDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime, int32 TimeStepHours)
{
	auto& Cells = SimulationActor->GetCells();

	auto SimulationSpan = EndTime - StartTime;
	int32 SimulationHours = SimulationSpan.GetTotalHours();

	FDateTime Time = StartTime;

	MaxSnow = 0;

	for (int32 Hours = 0; Hours < SimulationHours; Hours += TimeStepHours)
	{
		// Simulation
		for (auto& Cell : Cells)
		{
			auto NextStep = Time + FTimespan(TimeStepHours, 0, 0);
			const FVector& CellCentroid = Cell.Centroid;
			FTemperature Temperature = Data->GetTemperatureData(Time, NextStep, FVector2D(CellCentroid.X, CellCentroid.Y), SimulationActor, TimeStepHours); // @TODO timesteps
			
			const float TAir = Interpolator->InterpolateTemperatureByAltitude(Temperature, CellCentroid).Average; // degree Celsius
			const float Precipitation = Data->GetPrecipitationAt(Time, NextStep, FVector2D(CellCentroid.X, CellCentroid.Y), TimeStepHours); // l/m^2 or mm // @TODO timesteps
			
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
					Cell.SnowAlbedo = 0.4 * (1 + FMath::Exp(-k_e * Cell.DaysSinceLastSnowfall)); // @TODO is time T the degree-days or the time since the last snowfall?;
				}

				// Temperature higher than melt threshold and cell contains snow
				if (TAir > TMeltA)
				{
					const float DayNormalization = 24.0f / TimeStepHours; // day 

					// @TODO radiation index at nighttime? How about newer simulations?
					// @TODO Blöschl (???) used different radiation values at night time

					// Radiation Index
					const float R_i = SolarRadiationIndex(Cell.Inclination, Cell.Aspect, Cell.Latitude, Time.GetDayOfYear()); // 1

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

			Cell.DaysSinceLastSnowfall += 24.0f / TimeStepHours;
		}

		// Store max snow
		for (auto& Cell : Cells)
		{
			auto AreaSquareMeters = Cell.Area / (100 * 100);
			MaxSnow = FMath::Max(Cell.SnowWaterEquivalent / AreaSquareMeters, MaxSnow);
		}

		Time += FTimespan(TimeStepHours, 0, 0);
	}
}

void UDegreeDayCPUSimulation::Initialize(TArray<FSimulationCell>& Cells, USimulationWeatherDataProviderBase* Data)
{
	// @TODO discrete second derivative
}

#if SIMULATION_DEBUG
void UDegreeDayCPUSimulation::RenderDebug(TArray<FSimulationCell>& Cells, UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType)
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
				default:
					break;
				}
			}
		}

		Index++;
	}
}

float UDegreeDayCPUSimulation::GetMaxSnow()
{
	return MaxSnow;
}

#endif // SIMULATION_DEBUG


