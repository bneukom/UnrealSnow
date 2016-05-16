#include "SnowSimulation.h"
#include "SimulationWeatherDataProviderBase.h"
#include "RichardsonWeatherDataProvider.h"

FTemperature URichardsonWeatherDataProvider::GetTemperatureData(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	return FTemperature();
}

float URichardsonWeatherDataProvider::GetPrecipitationAt(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	return 0.0f;
}