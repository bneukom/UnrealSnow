#include "SnowSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SimulationWeatherDataProviderBase.h"
#include "WorldClimWeatherDataProvider.h"

FTemperature UWorldClimWeatherDataProvider::GetTemperatureData(const FDateTime& Date, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution)
{
	const float R = 6378137;

	
	//Coordinate offsets in radians
	float LatOffsetRadians = Position.Y / 100.0f / R;
	float LonOffsetRadians = Position.X / 100.0f / (R*FMath::Cos(PI* Simulation->Latitude / 180));

	// Divide data by 10 because WorldClim stores them as 16 bit integer times 10
	int16 Temperature = WorldClimData[0]->GetDataAt(Simulation->Latitude + LatOffsetRadians * 180 / PI, Simulation->Longitude + LonOffsetRadians * 180 / PI) / 10.0f;
	
	// @TODO convert Position to lat long
	return FTemperature(Temperature, Temperature, Temperature);
}

float UWorldClimWeatherDataProvider::GetPrecipitationAt(const FDateTime& Date, const FVector2D& Position, int64 Resolution)
{
	return 0.0f;
}