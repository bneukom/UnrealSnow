#include "SimulationData.h"
#include "SimulationWeatherDataProviderBase.h"
#include "MeteoSwissWeatherDataProvider.h"

TResourceArray<FClimateData>* UMeteoSwissWeatherDataProvider::CreateRawClimateDataResourceArray(FDateTime StartTime, FDateTime EndTime)
{
	TResourceArray<FClimateData>* ClimateDataResourceArray = new TResourceArray<FClimateData>();
	ClimateDataResourceArray->Reserve(ClimateData.Num());

	for (const FClimateData& Data : ClimateData)
	{
		ClimateDataResourceArray->Add(Data);
	}

	return ClimateDataResourceArray;
}

void UMeteoSwissWeatherDataProvider::Initialize(FDateTime StartTime, FDateTime EndTime)
{
	auto CurrentTime = StartTime;
	auto SimulationTime = EndTime - StartTime;
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
