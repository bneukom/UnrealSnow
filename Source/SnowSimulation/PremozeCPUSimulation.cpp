// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "PremozeCPUSimulation.h"

FString UPremozeCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Premoze CPU"));
}
// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UPremozeCPUSimulation::Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, FDateTime& StartTime, FDateTime& EndTime)
{
	
	auto SimulationSpan = EndTime - StartTime;
	auto SimulationHours = SimulationSpan.GetHours();

	FDateTime Time = StartTime;
	for (int Hours = 0; Hours < SimulationHours; ++Hours)
	{
		Time += FTimespan(Hours,0,0);
		for (auto& Cell : Cells)
		{
			const FVector& CellCentroid = Cell.Centroid;
			const float TAir = Data->GetDailyTemperatureData(Time.GetDayOfYear(), FVector2D(CellCentroid.X, CellCentroid.Y)).Mean + TemperatureDecay * Cell.Altitude;
			
			// Temperature bigger than melt threshold
			if (TAir > TMelt)
			{
				const float k_v = FMath::Exp(-4 * Data->GetVegetationDensityAt(Cell.Centroid));
				const float A = 0.4 * (1 + FMath::Exp(-k_e * Time.GetDayOfYear())); // @TODO is time T the degree-days or the time since the last snowfall?

				// Calculate radiation index
				const float DayOfTheYear = Time.GetDayOfYear();
				const float R_i = SolarRadiationIndex(Cell.Inclination, Cell.Aspect, Cell.Latitude, DayOfTheYear);

				// Calculate melt factor
				const float c_m = k_m * k_v * (1 - A);
			}
		
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


