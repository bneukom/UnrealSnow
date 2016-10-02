#include "SimulationData.h"
#include "WorldClimWeatherDataProvider.h"

TResourceArray<FClimateData>* UWorldClimWeatherDataProvider::CreateRawClimateDataResourceArray(FDateTime StartTime, FDateTime EndTime)
{
	return nullptr;
}

void UWorldClimWeatherDataProvider::Initialize(FDateTime StartTime, FDateTime EndTime)
{
}
