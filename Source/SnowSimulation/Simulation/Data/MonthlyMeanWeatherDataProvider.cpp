// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "MonthlyMeanWeatherDataProvider.h"

// @TODO At the beginning create temperature as well as precipitation data from the input.

FTemperature UMonthlyMeanSimulationDataProvider::GetTemperatureData(const FDateTime& Date, const FDateTime& To, const FVector2D& Position, ASnowSimulationActor* Simulation, int64 Resolution)
{
	auto MonthIndex = Date.GetMonth() - 1;
	auto AverageMonthlyTemperature = MonthlyTemperatures[MonthIndex];

	return AverageMonthlyTemperature;
}

float UMonthlyMeanSimulationDataProvider::GetPrecipitationAt(const FDateTime& Date, const FDateTime& To, const FVector2D& Position, int64 Resolution)
{
	auto MonthIndex = Date.GetMonth() - 1;
	auto Precipitation = MonthlyPrecipitation[MonthIndex];

	return Precipitation.AveragePrecipitation;
}

