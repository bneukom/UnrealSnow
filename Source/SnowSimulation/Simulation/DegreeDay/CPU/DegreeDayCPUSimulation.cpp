// Fill out your copyright notice in the Description page of Project Settings.


#include "SnowSimulation.h"
#include "DegreeDayCPUSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Util/MathUtil.h"
#include "SnowSimulation/Simulation/Interpolation/SimulationDataInterpolatorBase.h"

FString UDegreeDayCPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day CPU"));
}

// @TODO calculate discrete second derivative (curvature) for Blöschl snow approximation
// @TODO Use Fearings stability method for small scale snow?
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
					// @TODO is time T the degree-days or the time since the last snowfall?
					Cell.SnowAlbedo = 0.4 * (1 + FMath::Exp(-k_e * Cell.DaysSinceLastSnowfall)); 
				}

				// Temperature higher than melt threshold and cell contains snow
				if (TAir > TMeltA)
				{
					const float DayNormalization = 24.0f / TimeStepHours; // day 

					// @TODO radiation index at nighttime? How about newer simulations?
					// @TODO Blöschl (???) used different radiation values during night

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

		// Interpolation according to Blöschls "Distributed Snowmelt Simulations inan Alpine Catchment"
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

		Time += FTimespan(TimeStepHours, 0, 0);
	}
}

void UDegreeDayCPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, UWorld* World)
{
	auto Scale = SimulationActor->LandscapeScale;

	// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
	const float L = SimulationActor->LandscapeScale.X / 100 * SimulationActor->CellSize;

	// Calculate curvature as described in "Quantitative Analysis of Land Surface Topography" by Zevenbergen and Thorne.
	for (auto& Cell : SimulationActor->GetCells())
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


