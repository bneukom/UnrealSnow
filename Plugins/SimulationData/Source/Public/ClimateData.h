#pragma once

/** Weather data (precipitation and temperature). */
struct FClimateData {
	float Temperature;
	float Precipitation;

	FClimateData(float Precipitation, float Temperature) : Precipitation(Precipitation), Temperature(Temperature)
	{
	}

	FClimateData() : Precipitation(0.0f), Temperature(0.0f)
	{
	}
};
