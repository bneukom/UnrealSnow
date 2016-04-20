// Fill out your copyright notice in the Description page of Project Settings.

#include "SnowSimulation.h"
#include "SimulationDataProviderBase.h"
#include "DefaultDataInterpolator.h"


FTemperature UDefaultSimulationDataInterpolator::GetInterpolatedTemperatureData(FTemperature& BaseTemperatur, const FVector& Position)
{
	float Altitude = Position.Z; // Altitude in cm
	float Decay = TemperatureDecay * Altitude / (100 * 100);
	return FTemperature(BaseTemperatur.AverageLow + Decay, BaseTemperatur.AverageHigh + Decay, BaseTemperatur.Average + Decay);
}
