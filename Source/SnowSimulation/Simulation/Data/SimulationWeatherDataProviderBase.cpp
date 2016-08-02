
#include "SnowSimulation.h"
#include "SimulationWeatherDataProviderBase.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"

FClimateData USimulationWeatherDataProviderBase::GetInterpolatedClimateData(const FDateTime& TimeStamp, const FVector2D& Position)
{
	const ASnowSimulationActor* Owner = Cast<ASnowSimulationActor>(GetOwner());

	const float CellSizeX = Owner->OverallResolution * Owner->LandscapeScale.X / Owner->CellsDimension;
	const float CellSizeY = Owner->OverallResolution * Owner->LandscapeScale.Y / Owner->CellsDimension;

	const int32 IndexX = static_cast<int32>(Position.X / CellSizeX);
	const int32 IndexY = static_cast<int32>(Position.Y / CellSizeY);

	return GetInterpolatedClimateData(TimeStamp, IndexX, IndexY);
}

