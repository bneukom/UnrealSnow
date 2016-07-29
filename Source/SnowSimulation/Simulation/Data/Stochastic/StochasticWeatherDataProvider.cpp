#include "SnowSimulation.h"
#include "Simulation/SnowSimulationActor.h"
#include "Simulation/Data/SimulationWeatherDataProviderBase.h"
#include "StochasticWeatherDataProvider.h"
#include "SimplexNoiseBPLibrary.h"
#include "UnrealMathUtility.h"

UStochasticWeatherDataProvider::UStochasticWeatherDataProvider()
{

}

void UStochasticWeatherDataProvider::Initialize()
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());

	// Initial state
	State = (FMath::FRand() < P_I_W) ? WeatherState::WET : WeatherState::DRY;

	// Generate Temperature Noise which is assumed to be constant
	const float TemperatureNoiseScale = 0.01f;
	auto TemperatureNoise = std::vector<std::vector<float>>(Resolution, std::vector<float>(Resolution));
	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			float Noise = USimplexNoiseBPLibrary::SimplexNoiseScaled2D(X * TemperatureNoiseScale, Y * TemperatureNoiseScale, 2.0f);
			TemperatureNoise[X][Y] = Noise;
		}
	}

	// Generate precipitation
	auto TimeSpan = Simulation->EndTime - Simulation->StartTime;
	FDateTime CurrentTime = Simulation->StartTime;

	ClimateData = std::vector<TResourceArray<FWeatherData>*>();
	auto Measurement = std::vector<std::vector<float>>(Resolution, std::vector<float>(Resolution));

	for (int32 Hour = 0; Hour < TimeSpan.GetTotalHours(); ++Hour)
	{
		// Generate weather data
		auto ClimateDataHour = new TResourceArray<FWeatherData>();
		ClimateDataHour->Empty(Resolution * Resolution);
		ClimateDataHour->SetAllowCPUAccess(true);

		for (int32 Y = 0; Y < Resolution; ++Y)
		{
			for (int32 X = 0; X < Resolution; ++X)
			{
				float Precipitation = 0.0f;

				// Precipitation
				if (State == WeatherState::WET)
				{
					// Generate Precipitation noise for each station
					const float PrecipitationNoiseScale = 0.01;
					for (int32 Y = 0; Y < Resolution; ++Y)
					{
						for (int32 X = 0; X < Resolution; ++X)
						{
							float Noise = USimplexNoiseBPLibrary::SimplexNoiseScaled2D(X * PrecipitationNoiseScale, Y * PrecipitationNoiseScale, 0.5f) + 0.5f;
							Measurement[X][Y] = Noise;
						}
					}

					const float RainFallMM = 2.5f * FMath::Exp(2.5f * FMath::FRand()) / 24.0f;
					Precipitation = RainFallMM * Measurement[X][Y];
				}

				// Temperature
				float SeasonalOffset = -FMath::Cos(CurrentTime.GetDayOfYear() * 2 * PI / 365.0f) * 9 + FMath::FRandRange(-0.5f, 0.5f);
				const float BaseTemperature = 10;
				const float T = BaseTemperature + SeasonalOffset + TemperatureNoise[X][Y];

				ClimateDataHour->Add(FWeatherData(Precipitation, T));
			}
		}

		ClimateData.push_back(ClimateDataHour);

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
		USimplexNoiseBPLibrary::SetNoiseSeed(FMath::Rand());
	}
}

FWeatherData UStochasticWeatherDataProvider::GetInterpolatedClimateData(const FDateTime& TimeStamp, int SimulationIndexX, int SimulationIndexY)
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto TimeStepTimespan = TimeStamp - Simulation->StartTime;

	int32 IndexX = static_cast<int32>(SimulationIndexX / (float)Simulation->CellsDimension * Resolution);
	int32 IndexY = static_cast<int32>(SimulationIndexY / (float)Simulation->CellsDimension * Resolution);

	auto Data = *ClimateData[TimeStepTimespan.GetTotalHours()];

	int WeatherCellSize = Simulation->CellsDimension / Resolution;
		
	float GridIndexX = SimulationIndexX % WeatherCellSize;
	float GridIndexY = SimulationIndexY % WeatherCellSize;

	// Bilinear interpolation
	// @TODO assume measurements are top left corner

	auto Original = Data[IndexX + Resolution * IndexY];
	return FWeatherData(InterpolatedPrecipitation, Original.Temperature);
}

TResourceArray<FWeatherData>* UStochasticWeatherDataProvider::GetRawClimateData(const FDateTime& TimeStamp)
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto TimeStepTimespan = TimeStamp - Simulation->StartTime;

	return ClimateData[TimeStepTimespan.GetTotalHours()];
}

int32 UStochasticWeatherDataProvider::GetResolution()
{
	return Resolution;
}
