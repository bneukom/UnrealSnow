#include "SnowSimulation.h"
#include "Simulation/SnowSimulationActor.h"
#include "Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "SimpleStochasticWeatherDataProvider.h"
#include "SimplexNoiseBPLibrary.h"

USimpleStochasticWeatherDataProvider::USimpleStochasticWeatherDataProvider()
{

}

FTemperature USimpleStochasticWeatherDataProvider::GetTemperatureData(const FDateTime& From, const FDateTime& To, const FVector2D& Position, ASnowSimulationActor* SnowSimulation, int64 Resolution)
{
	auto StepDuration = To - From;

	float TotalTemperature = 0.0f;
	for (int Hour = 0; Hour < StepDuration.GetTotalHours(); ++Hour)
	{
		const ASnowSimulationActor* Owner = Cast<ASnowSimulationActor>(GetOwner());
		const auto TimeDiff = From - Owner->StartTime;

		const float CellSizeX = Owner->OverallResolution * Owner->LandscapeScale.X / Owner->CellsDimension;
		const float CellSizeY = Owner->OverallResolution * Owner->LandscapeScale.Y / Owner->CellsDimension;

		const int32 IndexX = static_cast<int32>(Position.X / CellSizeX);
		const int32 IndexY = static_cast<int32>(Position.Y / CellSizeY);

		TotalTemperature += TemperatureData[TimeDiff.GetTotalHours() + Hour].Average + TemperatureNoise[IndexX][IndexY];
	}

	TotalTemperature /= StepDuration.GetTotalHours();

	return FTemperature(TotalTemperature, TotalTemperature, TotalTemperature);
}

float USimpleStochasticWeatherDataProvider::GetPrecipitationAt(const FDateTime& From, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	auto StepDuration = To - From;

	float TotalPrecpitation = 0.0f;
	for (int Hour = 0; Hour < StepDuration.GetTotalHours(); ++Hour)
	{
		const ASnowSimulationActor* Owner = Cast<ASnowSimulationActor>(GetOwner());
		const auto TimeDiff = From - Owner->StartTime;

		const float CellSizeX = Owner->OverallResolution * Owner->LandscapeScale.X / Owner->CellsDimension;
		const float CellSizeY = Owner->OverallResolution * Owner->LandscapeScale.Y / Owner->CellsDimension;

		const int32 IndexX = static_cast<int32>(Position.X / CellSizeX);
		const int32 IndexY = static_cast<int32>(Position.Y / CellSizeY);

		TotalPrecpitation += PrecipitationData[TimeDiff.GetTotalHours() + Hour].GetPrecipitationAt(IndexX, IndexY);
	}

	return TotalPrecpitation;
}

void USimpleStochasticWeatherDataProvider::Initialize()
{
	const ASnowSimulationActor* Owner = Cast<ASnowSimulationActor>(GetOwner());
	const int32 Resolution = Owner->CellsDimension;

	// Initial state
	State = (FMath::FRand() < P_I_W) ? WeatherState::WET : WeatherState::DRY;

	// Generate Temperature Noise which is assumed to be constant
	const float TemperatureNoiseScale = 0.01f;
	TemperatureNoise = std::vector<std::vector<float>>(Resolution, std::vector<float>(Resolution));
	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			float Noise = USimplexNoiseBPLibrary::SimplexNoiseScaled2D(X * TemperatureNoiseScale, Y * TemperatureNoiseScale, 2.0f);
			TemperatureNoise[X][Y] = Noise;
		}
	}

	// Generate precipitation
	auto TimeSpan = Owner->EndTime - Owner->StartTime;
	FDateTime CurrentTime = Owner->StartTime;

	for (int32 Hour = 0; Hour < TimeSpan.GetTotalHours(); ++Hour)
	{
		// Precipitation
		if (State == WeatherState::WET)
		{
			// Exponential precipitation distribution function, divide by 24 because this is typically daily precipitation
			float RainFallMM = 2.5f * FMath::Exp(2.5f * FMath::FRand()) / 24.0f;

			FNoisePrecipitation Precipitation(RainFallMM, Resolution);

			const float PrecipitationNoiseScale = 0.01;

			// @TODO Less noise for better performance then linear interpolate
			// Generate Noise
			for (int32 Y = 0; Y < Resolution; ++Y)
			{
				for (int32 X = 0; X < Resolution; ++X)
				{
					float Noise = USimplexNoiseBPLibrary::SimplexNoiseScaled2D(X * PrecipitationNoiseScale, Y * PrecipitationNoiseScale, 0.5f) + 0.5f;
					Precipitation.Noise[X][Y] = Noise;
				}
			}

			PrecipitationData.Add(std::move(Precipitation));

			USimplexNoiseBPLibrary::SetNoiseSeed(FMath::Rand());
		}
		else {
			PrecipitationData.Add(FNoisePrecipitation());
		}

		// @TODO which paper is this from? (Original paper?)
		// Temperature
		float SeasonalOffset = -FMath::Cos(CurrentTime.GetDayOfYear() * 2 * PI / 365.0f) * 9 + FMath::FRandRange(-0.5f, 0.5f);
		const float BaseTemperature = 10;
		const float T = BaseTemperature + SeasonalOffset;
		TemperatureData.Add(FTemperature(T, T, T));

		// Next state
		WeatherState NextState;
		switch (State)
		{
		case WeatherState::WET:
			NextState = (FMath::FRand() < P_WW) ? WeatherState::WET : WeatherState::DRY;
			break;
		case WeatherState::DRY:
			NextState = (FMath::FRand() < P_WD) ? WeatherState::WET : WeatherState::DRY;
			break;
		default:
			NextState = WeatherState::DRY;
			break;
		}

		State = NextState;

		CurrentTime += FTimespan(1, 0, 0);


	}
}
