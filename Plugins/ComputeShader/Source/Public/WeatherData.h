#pragma once

/** Weather data (precipitation and temperature). */
struct FWeatherData {
	float Temperature;
	float Precipitation;

	FWeatherData(float Precipitation, float Temperature) : Precipitation(Precipitation), Temperature(Temperature)
	{
	}

	FWeatherData() : Precipitation(0.0f), Temperature(0.0f)
	{
	}
};
