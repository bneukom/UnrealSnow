#include "SnowSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "WorldClimWeatherDataProvider.h"

FWeatherData UWorldClimWeatherDataProvider::GetInterpolatedClimateData(const FDateTime& TimeStamp, int IndexX, int IndexY)
{
	// Earth radius approximation
	const float R = 6378137;
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());

	// @TODO wrong offset calculation with Index!
	// Coordinate offsets in radians
	float LatOffsetRadians = IndexY * 1 / R;
	float LonOffsetRadians = IndexX * 1 / (R*FMath::Cos(PI * Simulation->Latitude / 180));

	// @TODO access correct month
	// Divide data by 10 because WorldClim stores them as 16 bit integer times 10
	float Temperature = MonthlyData[0]->MeanTemperature->GetDataAt(Simulation->Latitude + LatOffsetRadians * 180 / PI, Simulation->Longitude + LonOffsetRadians * 180 / PI) / 10.0f;
	
	// @TODO implement precipitation
	// @TODO convert Position to lat long
	return FWeatherData(0.0f, Temperature);
}

TResourceArray<FWeatherData>* UWorldClimWeatherDataProvider::GetRawClimateData(const FDateTime& TimeStamp)
{
	return nullptr;
}

int32 UWorldClimWeatherDataProvider::GetResolution()
{
	return Resolution;
}

void UWorldClimWeatherDataProvider::Initialize()
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	Resolution = Simulation->CellsDimension;
}
