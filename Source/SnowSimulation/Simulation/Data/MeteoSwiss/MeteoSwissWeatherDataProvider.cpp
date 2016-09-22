#include "SnowSimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "MeteoSwissWeatherDataProvider.h"

FClimateData UMeteoSwissWeatherDataProvider::GetInterpolatedClimateData(const FDateTime& TimeStamp, int IndexX, int IndexY)
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto ElapsedTime = TimeStamp - Simulation->StartTime;
	auto ElapsedHours = ElapsedTime.GetTotalHours();
	return ClimateData[ElapsedHours];
}

TResourceArray<FClimateData>* UMeteoSwissWeatherDataProvider::CreateRawClimateDataResourceArray()
{
	TResourceArray<FClimateData>* ClimateDataResourceArray = new TResourceArray<FClimateData>();
	ClimateDataResourceArray->Reserve(ClimateData.Num());

	for (const FClimateData& Data : ClimateData)
	{
		ClimateDataResourceArray->Add(Data);
	}

	return ClimateDataResourceArray;
}

int32 UMeteoSwissWeatherDataProvider::GetResolution()
{
	return Resolution;
}

void UMeteoSwissWeatherDataProvider::Initialize()
{
	Resolution = 1;

	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto CurrentTime = Simulation->StartTime;
	auto SimulationTime = Simulation->EndTime - Simulation->StartTime;
	auto SimulationHours = SimulationTime.GetTotalHours();

	FString ContextString;
	for (int Hour = 0; Hour < SimulationHours; ++Hour)
	{
		auto YearString = FString::FromInt(CurrentTime.GetYear());
		auto MonthString = FString::Printf(TEXT("%02d"), CurrentTime.GetMonth());
		auto DayString = FString::Printf(TEXT("%02d"), CurrentTime.GetDay());
		auto HourString = FString::Printf(TEXT("%02d"), CurrentTime.GetHour());
		auto RowKey = YearString + MonthString + DayString + HourString;

		FTemperatureData* Temperature = TemperatureData->FindRow<FTemperatureData>(FName(*RowKey), ContextString);
		FPrecipitationData* Precipitation = PrecipitationData->FindRow<FPrecipitationData>(FName(*RowKey), ContextString);
		
		ClimateData.Push(FClimateData(Precipitation->Precipitation, Temperature->Temperature));
		CurrentTime += FTimespan(1, 0, 0);
	}
}

float UMeteoSwissWeatherDataProvider::GetMeasurementAltitude()
{
	return StationAltitude;
}
