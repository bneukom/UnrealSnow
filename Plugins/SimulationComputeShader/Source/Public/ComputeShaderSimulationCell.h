#pragma once

struct FComputeShaderSimulationCell
{
	float Aspect;
	float Inclination;
	float Altitude;
	float Latitude;
	float Area;
	float AreaXY;
	float SWE = 0.0f;
	float InterpolatedSWE;
	float SnowAlbedo = 0.0f;
	float DaysSinceLastSnowfall = 0.0f;
	float Curvature = 0.0f;

	FComputeShaderSimulationCell(float Aspect, float Inclination, float Altitude, float Latitude, float Area, float AreaXY) :
		Aspect(Aspect), Inclination(Inclination), Altitude(Altitude), Latitude(Latitude), Area(Area), AreaXY(AreaXY)
	{
	}
};