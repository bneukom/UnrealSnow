// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "PremozeCPUSimulation.h"
#include "SimulationDataInterpolatorBase.h"

FString UPremozeCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Premoze CPU"));
}
// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UPremozeCPUSimulation::Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime)
{
	// @TODO Test values from paper
	SolarRadiationIndex(FMath::DegreesToRadians(65), FMath::DegreesToRadians(0), FMath::DegreesToRadians(35), 174);

	auto SimulationSpan = EndTime - StartTime;
	int32 SimulationHours = SimulationSpan.GetTotalHours();

	FDateTime Time = StartTime;
	for (int32 Hours = 0; Hours < SimulationHours; Hours += TimeStepHours)
	{

		// @TODO only use one neighbor for snow distribution?
		for (auto& Cell : SlopeThresholdCells)
		{
			if (Cell.SnowWaterEquivalent > 0) {
				
				// Snow distribution function according to Blöschls paper "Distributed Snowmelt Simulations inan Alpine Catchme".
			}
		}

		Time += FTimespan(TimeStepHours, 0, 0);
		for (auto& Cell : Cells)
		{
			const FVector& CellCentroid = Cell.Centroid;
			FTemperature Temperature = Data->GetTemperatureData(Time, Time + FTimespan(TimeStepHours, 0,0), FVector2D(CellCentroid.X, CellCentroid.Y), ETimespan::TicksPerHour);
			
			const float TAir = Interpolator->GetInterpolatedTemperatureData(Temperature, CellCentroid).Average; // degree Celsius
			const float Precipitation = Data->GetPrecipitationAt(Time, Time + FTimespan(TimeStepHours, 0, 0), FVector2D(CellCentroid.X, CellCentroid.Y), ETimespan::TicksPerHour); // l/m^2 or mm
			
			const float AreaSquareMeters = Cell.Area / (100 * 100); // m^2

			if (Precipitation > 0)
			{
				Cell.DaysSinceLastSnowfall = 0;

				// New snow/rainfall
				const bool Rain = TAir > TSnow;

				if (Rain) 
				{
					Cell.SnowAlbedo = 0.4; // New rain drops the albedo to 0.4
				}
				else 
				{
					Cell.SnowWaterEquivalent += (Precipitation * AreaSquareMeters); // l/m^2 * m^2 = l
					Cell.SnowAlbedo = 0.8; // New snow sets the albedo to 0.8
				}
			}

			// Cell contains snow
			if (Cell.SnowWaterEquivalent > 0)
			{
				if (Cell.DaysSinceLastSnowfall >= 0) {
					Cell.SnowAlbedo = 0.4 * (1 + FMath::Exp(-k_e * Cell.DaysSinceLastSnowfall)); // @TODO is time T the degree-days or the time since the last snowfall?;
				}

				// Temperature bigger than melt threshold and cell contains snow
				if (TAir > TMelt)
				{
					const float DayNormalization = TimeStepHours / 24.0f; // day

					// Radiation Index
					const float R_i = SolarRadiationIndex(Cell.Inclination, Cell.Aspect, Cell.Latitude, Time.GetDayOfYear()); // 1

					// Melt factor
					const float k_v = FMath::Exp(-4 * Data->GetVegetationDensityAt(Cell.Centroid)); // @TODO unit?
					const float c_m = k_m * k_v * R_i *  (1 - Cell.SnowAlbedo) * DayNormalization * AreaSquareMeters; // l/m^2/C°/day * day * m^2 = l/m^2 * 1/day * day * m^2 = l/C°
					const float M = c_m * (TAir - TMelt); // l/C° * C° = l

					// Apply melt
					Cell.SnowWaterEquivalent -= M; 
					Cell.SnowWaterEquivalent = FMath::Max(0.0f, Cell.SnowWaterEquivalent);
				}
			}

			Cell.DaysSinceLastSnowfall++;
		}
	}
}

void UPremozeCPUSimulation::Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data)
{
	// Retains only those cells which have a neighbor with a lower z value which exceeds the slope threshold.
	SlopeThresholdCells = Cells.FilterByPredicate([this](const FSimulationCell& Cell) {
		bool Retain = false;
		for (auto NeighbourIndex = 0; NeighbourIndex < 8; NeighbourIndex++)
		{
			if (Cell.Neighbours[NeighbourIndex])
			{
				
				if (Cell.Centroid.Z < Cell.Neighbours[NeighbourIndex]->Centroid.Z) {
					continue;
				}
				const FVector& P0 = Cell.Centroid;
				const FVector& P1 = Cell.Neighbours[NeighbourIndex]->Centroid;


				const FVector Neighbour = P1 - P0;
				const FVector NeighbourProjXY(Neighbour.X, Neighbour.Y, 0);
				const float Angle = FMath::Abs(FMath::Acos(FVector::DotProduct(Neighbour, NeighbourProjXY) / (Neighbour.Size() * NeighbourProjXY.Size())));
				
				// Found
				if (Angle >= SlopeThreshold)
				{
					Retain = true;
					break;
				}
			}
		}
		return Retain;
	});

	// Sort the remaining cells by altitude for processing in the simulation
	SlopeThresholdCells.Sort([](const FSimulationCell& A, const FSimulationCell& B)
	{
		return A.Altitude < B.Altitude;
	});
}

#if SIMULATION_DEBUG
void UPremozeCPUSimulation::RenderDebug(TArray<FSimulationCell>& Cells)
{
	for (auto& Cell : Cells)
	{
		// Draw neighbor slope
		for (auto NeighbourIndex = 0; NeighbourIndex < 8; NeighbourIndex++)
		{
			if (Cell.Neighbours[NeighbourIndex])
			{
				const FVector* P0;
				const FVector* P1;

				FVector zOffset(0, 0, 50);

				if (Cell.Centroid.Z < Cell.Neighbours[NeighbourIndex]->Centroid.Z) {
					P0 = &Cell.Centroid;
					P1 = &Cell.Neighbours[NeighbourIndex]->Centroid;
				}
				else {
					P1 = &Cell.Centroid;
					P0 = &Cell.Neighbours[NeighbourIndex]->Centroid;
				}

				const FVector Neighbour = *P1 - *P0;
				const FVector NeighbourProjXY(Neighbour.X, Neighbour.Y, 0);
				const float Angle = FMath::Abs(FMath::Acos(FVector::DotProduct(Neighbour, NeighbourProjXY) / (Neighbour.Size() * NeighbourProjXY.Size())));
				const float MaxAngle = PI / 2;
				const float R = (255 * Angle) / MaxAngle;
				const float G = (255 * (MaxAngle - Angle)) / MaxAngle;

				if (Angle > PI / 6)
				{
					DrawDebugLine(GetWorld(), *P0 + zOffset, *P1 + zOffset, FColor(R, G, 0, 25), false, -1, 0, 0.0f);
				}
			}
		}
	}
}
#endif // SIMULATION_DEBUG


