#pragma once

struct FComputeShaderSimulationCell
{
	float Aspect;
	float Inclination;
	float Altitude;
	float Latitude;
	float Area;
	float AreaXY;
	float SnowWaterEquivalent;
	float InterpolatedSWE = 0.0f;
	float SnowAlbedo = 0.8f;
	float DaysSinceLastSnowfall = 0.0f;
	float Curvature = 0.0f;

	FComputeShaderSimulationCell(float Aspect, float Inclination, float Altitude, float Latitude, float Area, float AreaXY, float SnowWaterEquivalent = 0.0f) :
		Aspect(Aspect), Inclination(Inclination), Altitude(Altitude), Latitude(Latitude), Area(Area), AreaXY(AreaXY), SnowWaterEquivalent(SnowWaterEquivalent)
	{
	}
};