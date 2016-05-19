#include "SnowSimulation.h"
#include "SimulationWeatherDataProviderBase.h"
#include "RichardsonWeatherDataProvider.h"
#include <cmath>

URichardsonWeatherDataProvider::URichardsonWeatherDataProvider()
{
	// @TODO generate probabilities from observation

	// Initial state
	State = (FMath::FRand() < P_WI) ? WeatherState::WET : WeatherState::DRY;

	// Generate precipitation
	auto TimeSpan = To - From;
	for (int32 Day = 0; Day < TimeSpan.GetDays(); ++Day)
	{
		// Generate precipitation
		

		// Next state
		State NextState;
		switch (State)
		{
		case WeatherState::WET:
			NextState = (FMath::FRand() < P_WW) ? WeatherState::WET : WeatherState::DRY;
			break;
		case WeatherState::DRY:
			NextState = (FMath::FRand() < P_WD) ? WeatherState::WET : WeatherState::DRY;
			break;
		default:
			break;
		}
	}
}

FTemperature URichardsonWeatherDataProvider::GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution)
{
	return FTemperature();
}

float URichardsonWeatherDataProvider::GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution)
{
	return 0.0f;
}