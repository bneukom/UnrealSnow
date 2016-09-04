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
	auto TimeSpanHours =  static_cast<int32>(TimeSpan.GetTotalHours());
	FDateTime CurrentTime = Simulation->StartTime;

	ClimateData = std::vector<std::vector<FClimateData>>(TimeSpanHours, std::vector<FClimateData>(Resolution * Resolution));

	auto Measurement = std::vector<std::vector<float>>(Resolution, std::vector<float>(Resolution));

	for (int32 Hour = 0; Hour < TimeSpanHours; ++Hour)
	{
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
							float Noise = FMath::Max(USimplexNoiseBPLibrary::SimplexNoiseScaled2D(X * PrecipitationNoiseScale, Y * PrecipitationNoiseScale, 0.9f) + 0.2f, 0.0f);
							Measurement[X][Y] = Noise;
						}
					}

					// @TODO paper?
					const float RainFallMM = 2.5f * FMath::Exp(2.5f * FMath::FRand()) / 24.0f;
					Precipitation = RainFallMM * Measurement[X][Y];
				}

				// @TODO paper?
				// Temperature
				float SeasonalOffset = -FMath::Cos(CurrentTime.GetDayOfYear() * 2 * PI / 365.0f) * 9 + FMath::FRandRange(-0.5f, 0.5f);
				const float BaseTemperature = 10;
				const float OvercastTemperatureOffset = State == WeatherState::WET ? -8 : 0;
				const float T = BaseTemperature + SeasonalOffset + OvercastTemperatureOffset + TemperatureNoise[X][Y];

				ClimateData[Hour][X + Y * Resolution] = FClimateData(Precipitation, T);
			}
		}

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

FClimateData UStochasticWeatherDataProvider::GetInterpolatedClimateData(const FDateTime& TimeStamp, int SimulationIndexX, int SimulationIndexY)
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto TimeStepTimespan = TimeStamp - Simulation->StartTime;

	int32 IndexX = static_cast<int32>(SimulationIndexX / (float)Simulation->CellsDimensionX * Resolution);
	int32 IndexY = static_cast<int32>(SimulationIndexY / (float)Simulation->CellsDimensionY * Resolution);

	int TimeStep = TimeStepTimespan.GetTotalHours();
	auto& Data = ClimateData[TimeStep];

	int WeatherCellSizeX = Simulation->CellsDimensionX / Resolution;
	int WeatherCellSizeY = Simulation->CellsDimensionY / Resolution;

	float GridIndexX = SimulationIndexX % (WeatherCellSizeX + 1);
	float GridIndexY = SimulationIndexY % (WeatherCellSizeY + 1);

	int XCellA = IndexX;
	int XCellB = FMath::Min(IndexX + 1, Resolution - 1);
	int YCellA = IndexY;
	int YCellB = FMath::Min(IndexY + 1, Resolution - 1);

	float AA = Data[XCellA + Resolution * YCellA].Precipitation;
	float AB = Data[XCellA + Resolution * YCellB].Precipitation;
	float BA = Data[XCellB + Resolution * YCellA].Precipitation;
	float BB = Data[XCellB + Resolution * YCellB].Precipitation;

	float Y1 = (1 - GridIndexY / WeatherCellSizeY);
	float Y2 = GridIndexY / WeatherCellSizeY;

	// Bilinear interpolation
	// @TODO assume measurements are top left corner
	float Lerp1 = Y1 * AA + Y2 * AB;
	float Lerp2 = Y1 * BA + Y2 * BB;

	float X1 = (1 - GridIndexX / WeatherCellSizeX);
	float X2 = GridIndexX / WeatherCellSizeX;

	float Precipitation = X1 * Lerp1 + X2 * Lerp2;

	auto Original = Data[IndexX + Resolution * IndexY];
	return FClimateData(Precipitation, Original.Temperature);
}

TResourceArray<FClimateData>* UStochasticWeatherDataProvider::CreateRawClimateDataResourceArray()
{
	const ASnowSimulationActor* Simulation = Cast<ASnowSimulationActor>(GetOwner());
	auto TimeSpan = Simulation->EndTime - Simulation->StartTime;
	int32 TotalHours = static_cast<int>(TimeSpan.GetTotalHours());

	TResourceArray<FClimateData>* ClimateResourceArray = new TResourceArray<FClimateData>();
	ClimateResourceArray->Reserve(TotalHours * Resolution * Resolution);

	for (int32 Hour = 0; Hour < TotalHours; ++Hour)
	{
		for (int32 ClimateIndex = 0; ClimateIndex < Resolution * Resolution; ++ClimateIndex)
		{
			ClimateResourceArray->Add(ClimateData[Hour][ClimateIndex]);
		}
	}

	return ClimateResourceArray;
}

int32 UStochasticWeatherDataProvider::GetResolution()
{
	return Resolution;
}
