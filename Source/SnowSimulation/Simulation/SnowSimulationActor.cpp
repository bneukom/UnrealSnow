#include "SnowSimulation.h"
#include "UnrealMathUtility.h"
#include "SnowSimulationActor.h"
#include "Util/MathUtil.h"
#include "Util/RuntimeMaterialChange.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RenderCommandFence.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeComponent.h"
#include "Runtime/Landscape/Classes/LandscapeInfo.h"
#include "Runtime/Landscape/Classes/LandscapeProxy.h"
#include "Runtime/Landscape/Public/LandscapeDataAccess.h" //#include "RuntimeLandscapeDataAccess.h"
#include "UObject/NameTypes.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"

DEFINE_LOG_CATEGORY(SimulationLog);

ASnowSimulationActor::ASnowSimulationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
}

void ASnowSimulationActor::BeginPlay()
{
	Super::BeginPlay();

	// Create the cells and the texture data for the material
	Initialize();

	// Initialize components
	ClimateDataComponent = Cast<USimulationWeatherDataProviderBase>(GetComponentByClass(USimulationWeatherDataProviderBase::StaticClass()));
	ClimateDataComponent->Initialize();

	// Initialize simulation
	Simulation->Initialize(this, GetWorld());
	UE_LOG(SimulationLog, Display, TEXT("Simulation type used: %s"), *Simulation->GetSimulationName());
	CurrentSimulationTime = StartTime;

	// Run simulation
	CurrentSleepTime = SleepTime;
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentSleepTime += DeltaTime;

	if (CurrentSleepTime >= SleepTime)
	{
		CurrentSleepTime = 0;

		// Simulate next step
		Simulation->Simulate(this, CurrentSimulationStep);

		// Update the snow material to reflect changes from the simulation
		UpdateMaterialTexture();
		SetScalarParameterValue(Landscape, TEXT("MaxSnow"), Simulation->GetMaxSnow());
			
		// Update timestep
		CurrentSimulationTime += FTimespan(1, 0, 0);
		CurrentSimulationStep++;
	}

	// Render debug information
	// if (DebugVisualizationType != EDebugVisualizationType::Nothing) Simulation->RenderDebug(Cells, GetWorld(), CellDebugInfoDisplayDistance, DebugVisualizationType);
	if (RenderGrid) DoRenderGrid();

}

void ASnowSimulationActor::DoRenderGrid()
{
	/*
	const auto Location = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();

	for (FSimulationCell& Cell : Cells)
	{
		FVector Normal(Cell.Normal);
		Normal.Normalize();

		// @TODO get exact position using the height map
		FVector zOffset(0, 0, 50);

		if (FVector::Dist(Cell.Centroid, Location) < CellDebugInfoDisplayDistance)
		{
			// @TODO implement custom rendering for better performance (DrawPrimitiveUP)
			// Draw Cells
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P2 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P3 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P2 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P3 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
		}
	}
	*/
}

void ASnowSimulationActor::Initialize()
{
	if (GetWorld())
	{
		auto Level = GetWorld()->PersistentLevel;
		TActorIterator<ALandscape> LandscapeIterator(GetWorld( ));
		
		Landscape = *LandscapeIterator;
		LandscapeScale = Landscape->GetActorScale();

		if (Landscape)
		{
			auto& LandscapeComponents = Landscape->LandscapeComponents;

			LandscapeSizeQuads = FMath::Sqrt(LandscapeComponents.Num() * (Landscape->ComponentSizeQuads) * (Landscape->ComponentSizeQuads));
			CellsDimension = LandscapeSizeQuads / CellSize - 1; // -1 because we create cells and use 4 vertices
			NumCells = CellsDimension * CellsDimension;
			OverallResolution = Landscape->SubsectionSizeQuads * FMath::Sqrt(LandscapeComponents.Num());

			UE_LOG(SimulationLog, Display, TEXT("Num components: %d"), LandscapeComponents.Num());
			UE_LOG(SimulationLog, Display, TEXT("Num subsections: %d"), Landscape->NumSubsections);
			UE_LOG(SimulationLog, Display, TEXT("SubsectionSizeQuads: %d"), Landscape->SubsectionSizeQuads);
			UE_LOG(SimulationLog, Display, TEXT("ComponentSizeQuads: %d"), Landscape->ComponentSizeQuads);
			UE_LOG(SimulationLog, Display, TEXT("LandscapeSizeQuads: %d"), LandscapeSizeQuads);
		}


		// Update shader
		SetScalarParameterValue(Landscape, TEXT("CellsDimension"), CellsDimension);
		SetScalarParameterValue(Landscape, TEXT("Resolution"), OverallResolution);
	}
}

void ASnowSimulationActor::UpdateMaterialTexture()
{
	auto SnowMapTexture = Simulation->GetSnowMapTexture();
	if (WriteDebugTextures)
	{
		auto SnowMapPath = DebugTexturePath + "\\SnowMap";
		FFileHelper::CreateBitmap(*SnowMapPath, CellsDimension, CellsDimension, Simulation->GetSnowMapTextureData().GetData());
	}
	SetTextureParameterValue(Landscape, TEXT("SnowMap"), SnowMapTexture, GEngine);
}

#if WITH_EDITOR
void ASnowSimulationActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//Get the name of the property that was changed  
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(ASnowSimulationActor, CellSize))) {
		Initialize();
	}
}
#endif

