#include "SnowSimulation.h"
#include "SimulationWeatherDataProviderBase.h"
#include "WorldClimWeatherDataProvider.h"

FTemperature UWorldClimWeatherDataProvider::GetTemperatureData(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	// @TODO convert Position to lat long
	From

	return FTemperature();
}

float UWorldClimWeatherDataProvider::GetPrecipitationAt(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	return 0.0f;
}