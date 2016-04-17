// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "DefaultDataProvider.h"



FTemperature UDefaultSimulationDataProvider::GetDailyTemperatureData(const int Day, const FVector2D& Position)
{
	return FTemperature();
}

float UDefaultSimulationDataProvider::GetPrecipitationAt(const FDateTime& Time, const FVector2D& Position)
{
	return 0.0f;
}

float UDefaultSimulationDataProvider::GetVegetationDensityAt(const FVector& Position)
{
	return 0.0f;
}
