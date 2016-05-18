#include "SnowSimulation.h"
#include "SimulationWeatherDataProviderBase.h"
#include "RichardsonWeatherDataProvider.h"

FTemperature URichardsonWeatherDataProvider::GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution)
{
	return FTemperature();
}

float URichardsonWeatherDataProvider::GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution)
{
	return 0.0f;
}