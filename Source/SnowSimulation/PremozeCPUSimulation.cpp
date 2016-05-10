// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "PremozeCPUSimulation.h"
#include "MathUtil.h"
#include "SimulationDataInterpolatorBase.h"

FString UPremozeCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Premoze CPU"));
}

// @TODO what is the time step of Premozes simulation?
// @TODO Test Solar Radiations values from the paper

// Flops per iteration: (2 * 20 + 6 * 2) + (20 * 20 + 38 * 2)
void UPremozeCPUSimulation::Simulate(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data, USimulationDataInterpolatorBase* Interpolator, FDateTime StartTime, FDateTime EndTime, int32 TimeStepHours)
{
	// @TODO run Premoze in daily timesteps, other timesteps do not make any sense!
	auto SimulationSpan = EndTime - StartTime;
	int32 SimulationHours = SimulationSpan.GetTotalHours();

	FDateTime Time = StartTime;

	MaxSWE = 0;

	// @TODO timesteps
	for (int32 Hours = 0; Hours < SimulationHours; Hours += TimeStepHours)
	{
		// Simulation
		Time += FTimespan(TimeStepHours, 0, 0);
		for (auto& Cell : Cells)
		{
			const FVector& CellCentroid = Cell.Centroid;
			FTemperature Temperature = Data->GetTemperatureData(Time, Time + FTimespan(TimeStepHours, 0, 0), FVector2D(CellCentroid.X, CellCentroid.Y), TimeStepHours * ETimespan::TicksPerHour); // @TODO timesteps
			
			const float TAir = Interpolator->GetInterpolatedTemperatureData(Temperature, CellCentroid).Average; // degree Celsius
			const float Precipitation = Data->GetPrecipitationAt(Time, Time + FTimespan(TimeStepHours, 0, 0), FVector2D(CellCentroid.X, CellCentroid.Y), TimeStepHours * ETimespan::TicksPerHour); // l/m^2 or mm // @TODO timesteps
			
			// @TODO use AreaXY because very steep slopes with big areas would receive too much snow
			const float AreaSquareMeters = Cell.AreaXY / (100 * 100); // m^2

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
					// @TODO modify how much snow a surface receives by aspect? A surface with a 90° slope should not receive much snow. (FIND PAPER)
					//		-  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.461.563&rep=rep1&type=pdf
					// @TODO if we assume no wind we could calculate the area by projecting the plane onto the XY plane

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

				// @TODO is this correct?
				// Temperature bigger than melt threshold and cell contains snow
				if (TAir > TMelt)
				{
					const float DayNormalization = 24.0f / TimeStepHours; // day 

					// @TODO radiation index at nighttime? How about newer simulations?
					// @TODO Blöschl (???) used different radiation values at night time

					// Radiation Index
					const float R_i = SolarRadiationIndex(Cell.Inclination, Cell.Aspect, Cell.Latitude, Time.GetDayOfYear()); // 1

					// Melt factor
					const float k_v = FMath::Exp(-4 * Data->GetVegetationDensityAt(Cell.Centroid)); // 1
					const float c_m = k_m * k_v * R_i *  (1 - Cell.SnowAlbedo) * DayNormalization * AreaSquareMeters; // l/m^2/C°/day * day * m^2 = l/m^2 * 1/day * day * m^2 = l/C°
					const float M = c_m * (TAir - TMelt); // l/C° * C° = l

					// Apply melt
					Cell.SnowWaterEquivalent -= M; 
					Cell.SnowWaterEquivalent = FMath::Max(0.0f, Cell.SnowWaterEquivalent);
				}
			}

			Cell.DaysSinceLastSnowfall += 24.0f / TimeStepHours;
		}

		TArray<FSimulationCell*> CurrentTestCells = StabilityTestCells;
		

		for (int StabilitTest = 0; StabilitTest < StabilityIterations; ++StabilitTest)
		{
			TArray<FSimulationCell*> UnstableCells;

			// Sort the cells by total altitude
			CurrentTestCells.Sort([](const FSimulationCell& A, const FSimulationCell& B) {
				return A.AltitudeWithSnow() > B.AltitudeWithSnow();
			});

			// Snow stability test similar to the method described in "Computer Modelling Of Fallen Snow"
			for (auto Cell : CurrentTestCells)
			{
				if (Cell->SnowWaterEquivalent > 0)
				{
					bool SnowAvalanched = false;

					for (int NeighbourIndex = 0; NeighbourIndex < 8; ++NeighbourIndex)
					{
						auto Neighbour = Cell->Neighbours[NeighbourIndex];
						const float InstableSnow = Cell->SnowWaterEquivalent * 1 / (StabilityIterations - StabilitTest);

						// @TODO for gullies the assumption that the neighbor snow altitude has to be lower is not correct.
						if (Neighbour != nullptr && Neighbour->AltitudeWithSnow() < Cell->AltitudeWithSnow())
						{
							auto CellAreaSquareMeters = Cell->Area / (100 * 100);
							auto CellSWE = Cell->SnowWaterEquivalent / CellAreaSquareMeters; // l/m^2 or mm
							auto CellSnowHeightOffset = Cell->Normal.GetSafeNormal() * CellSWE * 10;
							const FVector& P0 = Cell->Centroid + CellSnowHeightOffset;

							auto NeighbourAreaSquareMeters = Neighbour->Area / (100 * 100);
							auto NeighbourSWE = Neighbour->SnowWaterEquivalent / NeighbourAreaSquareMeters; // l/m^2 or mm
							auto NeighbourSnowHeightOffset = Neighbour->Normal.GetSafeNormal() * NeighbourSWE * 10;
							const FVector& P1 = Neighbour->Centroid + NeighbourSnowHeightOffset;

							const FVector ToNeighbour = P1 - P0;
							const FVector NeighbourProjXY(ToNeighbour.X, ToNeighbour.Y, 0);
							const float Slope = FMath::Abs(FMath::Acos(FVector::DotProduct(ToNeighbour, NeighbourProjXY) / (ToNeighbour.Size() * NeighbourProjXY.Size())));

							const float SupportProbability = FMath::Min(Slope < FMath::DegreesToRadians(50) ? 1 : 1 - (Slope / FMath::DegreesToRadians(60)), 1.0f);
							const bool Support = FMath::FRandRange(0.0f, 1.0f) <= SupportProbability;

							if (!Support) {
								// const float Avalanche = (Cell->SnowWaterEquivalent * (Slope / (PI / 2)) *  0.5f);
								const float Avalanche = Cell->SnowWaterEquivalent < 5 ? Cell->SnowWaterEquivalent : (Cell->SnowWaterEquivalent * (Slope / (PI / 2)) * 0.5f);
								
								Cell->SnowWaterEquivalent -= Avalanche;
								Cell->Neighbours[NeighbourIndex]->SnowWaterEquivalent += Avalanche;

								UnstableCells.Add(Cell->Neighbours[NeighbourIndex]);

								SnowAvalanched = true;

							}
						}
					}

					if (SnowAvalanched)
					{
						UnstableCells.Add(Cell);
					}
				}

			
			}

			CurrentTestCells = UnstableCells;
		}

		// Store max SWE
		for (auto& Cell : Cells)
		{
			MaxSWE = FMath::Max(Cell.SnowWaterEquivalent, MaxSWE);
		}
	}
}

void UPremozeCPUSimulation::Initialize(TArray<FSimulationCell>& Cells, USimulationDataProviderBase* Data)
{
	for (auto& Cell : Cells)
	{
		StabilityTestCells.Add(&Cell);
	}
}

#if SIMULATION_DEBUG
void UPremozeCPUSimulation::RenderDebug(TArray<FSimulationCell>& Cells, UWorld* World, int CellDebugInfoDisplayDistance)
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
			DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + FVector(0, 0, SWE * 10), FColor(255, 0, 0), false, -1, 0, 0.0f);
		}
	}

	const auto Location = World->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	auto PlayerController = World->GetFirstPlayerController();
	auto Pawn = PlayerController->GetPawn();

	// Render snow water equivalent
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
				// Total SWE
				DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.SnowWaterEquivalent)), nullptr, FColor::Purple, 0, true);
			}
		}
	}
}

float UPremozeCPUSimulation::GetMaxSWE()
{
	return MaxSWE;
}

#endif // SIMULATION_DEBUG


