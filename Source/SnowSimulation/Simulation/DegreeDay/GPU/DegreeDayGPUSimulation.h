#pragma once
// Fill out your copyright notice in the Description page of Project Settings.

#include "SimulationPixelShader.h"
#include "SimulationComputeShader.h"
#include "Simulation/DegreeDay/DegreeDaySimulation.h"
#include "SnowSimulation/Simulation/SimulationBase.h"
#include "DegreeDayGPUSimulation.generated.h"



/**
* Snow simulation similar to the one proposed by Simon Premoze in "Geospecific rendering of alpine terrain".
* Snow deposition is implemented similar to Fearings "Computer Modelling Of Fallen Snow".
*/
UCLASS(Blueprintable, BlueprintType)
class SNOWSIMULATION_API UDegreeDayGPUSimulation : public UDegreeDaySimulation
{
	GENERATED_BODY()

private:
	FSimulationComputeShader* SimulationComputeShader;

	FSimulationPixelShader* SimulationPixelShader;

	UTextureRenderTarget2D* RenderTarget;

	/**
	* Returns the cell at the given index or nullptr if the index is out of bounds.
	*
	* @param Index the index of the cell
	* @return the cell at the given index or nullptr if the index is out of bounds
	*/
	int GetCellChecked(int Index, int NumCells)
	{
		return (Index >= 0 && Index < NumCells) ? Index : -1;
	}

public:

	virtual FString GetSimulationName() override final;

	virtual void Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep) override final;

	virtual void Initialize(ASnowSimulationActor* SimulationActor, UWorld* World) override final;

	virtual UTexture* GetSnowMapTexture() override final;

	virtual TArray<FColor> GetSnowMapTextureData() override final;

	virtual float GetMaxSnow() override final;
};

