
#include "Simulation.h"
#include "LandscapeDataAccess.h"
#include "DegreeDayGPUSimulation.h"
#include "SnowSimulationActor.h"
#include "Util/MathUtil.h"
#include "LandscapeComponent.h"

FString UDegreeDayGPUSimulation::GetSimulationName()
{
	return FString(TEXT("Degree Day GPU"));
}

void UDegreeDayGPUSimulation::Simulate(ASnowSimulationActor* SimulationActor, int32 CurrentSimulationStep, int32 Timesteps, bool SaveSnowMap, bool CaptureDebugInformation, TArray<FDebugCell>& DebugCells)
{
	SimulationComputeShader->ExecuteComputeShader(CurrentSimulationStep, Timesteps, SimulationActor->CurrentSimulationTime.GetHour(), CaptureDebugInformation, DebugCells);
	SimulationPixelShader->ExecutePixelShader(RenderTarget, SaveSnowMap);
}

void UDegreeDayGPUSimulation::Initialize(ASnowSimulationActor* SimulationActor, const TArray<FLandscapeCell>& LandscapeCells, float InitialMaxSnow, UWorld* World)
{
	// Create shader
	SimulationComputeShader = new FSimulationComputeShader(World->Scene->GetFeatureLevel());
	SimulationPixelShader = new FSimulationPixelShader(World->Scene->GetFeatureLevel());

	// Create Cells
	TResourceArray<FGPUSimulationCell> Cells;
	for (const FLandscapeCell& LandscapeCell : LandscapeCells)
	{
		FGPUSimulationCell Cell(LandscapeCell.Aspect, LandscapeCell.Inclination, LandscapeCell.Altitude, 
			LandscapeCell.Latitude, LandscapeCell.Area, LandscapeCell.AreaXY, LandscapeCell.InitialWaterEquivalent);
		Cells.Add(Cell);
	}
	

	// Initialize render target
 	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(SimulationActor->CellsDimensionX, SimulationActor->CellsDimensionY);
	
	// Initialize shaders
	auto ClimateData = SimulationActor->ClimateDataComponent->CreateRawClimateDataResourceArray(SimulationActor->StartTime, SimulationActor->EndTime);
	auto SimulationTimeSpan = SimulationActor->EndTime - SimulationActor->StartTime;
	int32 TotalHours = static_cast<int32>(SimulationTimeSpan.GetTotalHours());

	SimulationComputeShader->Initialize(Cells, *ClimateData, k_e, k_m, TMeltA, TMeltB, TSnowA, TSnowB, TotalHours, 
		SimulationActor->CellsDimensionX, SimulationActor->CellsDimensionY, SimulationActor->ClimateDataComponent->GetMeasurementAltitude(), InitialMaxSnow);

	SimulationPixelShader->Initialize(SimulationComputeShader->GetSnowBuffer(), SimulationComputeShader->GetMaxSnowBuffer(), SimulationActor->CellsDimensionX, SimulationActor->CellsDimensionY);
}

UTexture* UDegreeDayGPUSimulation::GetSnowMapTexture()
{
	UTexture* RenderTargetTexture = Cast<UTexture>(RenderTarget);
	return  RenderTargetTexture;
}


float UDegreeDayGPUSimulation::GetMaxSnow()
{
	return SimulationComputeShader->GetMaxSnow();
}

