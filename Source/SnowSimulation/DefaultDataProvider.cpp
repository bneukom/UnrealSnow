// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "DefaultDataProvider.h"



FTemperature UDefaultSimulationDataProvider::GetTemperatureData(const FDateTime& From, const FDateTime& To, const FDateTime& Resolution, const FVector2D& Position)
{
	Timespan.
	auto MonthIndex = Time.GetMonth() - 1;
	return MonthlyTemperatures[MonthIndex];
}

float UDefaultSimulationDataProvider::GetPrecipitationAt(const FDateTime& Time, const FTimespan& Timespan, const FDateTime& Resolution, const FVector2D& Position)
{
	auto MonthIndex = Time.GetMonth() - 1;
	return MonthlyPrecipitation;
}

float UDefaultSimulationDataProvider::GetVegetationDensityAt(const FVector& Position)
{
	return 0.0f;
}
