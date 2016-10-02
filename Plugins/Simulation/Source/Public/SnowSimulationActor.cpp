#include "Simulation.h"
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
	ClimateDataComponent->Initialize(StartTime, EndTime);

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
		Simulation->Simulate(this, CurrentSimulationStep, Timesteps);

		// Update the snow material to reflect changes from the simulation
		UpdateMaterialTexture();
		SetScalarParameterValue(Landscape, TEXT("MaxSnow"), Simulation->GetMaxSnow());
			
		// Update timestep
		CurrentSimulationTime += FTimespan(Timesteps, 0, 0);
		CurrentSimulationStep += Timesteps;

		// Take screenshot
		if (SaveSimulationFrames)
		{

			FString FileName = "simulation_" + FString::FromInt(CurrentSimulationTime.GetYear()) + "_"
				+ FString::FromInt(CurrentSimulationTime.GetMonth()) + "_" + FString::FromInt(CurrentSimulationTime.GetDay()) + "_"
				+ FString::FromInt(CurrentSimulationTime.GetHour()) + ".png";

			FScreenshotRequest::RequestScreenshot(FileName, false, false);
		}
	}

	// Render debug information
	// if (DebugVisualizationType != EDebugVisualizationType::Nothing) Simulation->RenderDebug(Cells, GetWorld(), CellDebugInfoDisplayDistance, DebugVisualizationType);
	// if (RenderGrid) DoRenderGrid();

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

void ASnowSimulationActor::DoRenderDebugInformation()
{
	/*
	// Draw SWE normal
	if (DebugVisualizationType == EDebugVisualizationType::SnowHeight)
	{
		for (auto& Cell : Cells)
		{
			if (Cell.SnowWaterEquivalent > 0)
			{
				FVector Normal(Cell.Normal);
				Normal.Normalize();

				// @TODO get exact position using the height map
				FVector zOffset(0, 0, 50);

				// Height of the snow for this cell
				auto AreaSquareMeters = Cell.Area / (100 * 100);
				auto SWE = Cell.SnowWaterEquivalent / AreaSquareMeters; // l/m^2 or mm
																		//DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + zOffset + (Normal * SWE * 10), FColor(255, 0, 0), false, -1, 0, 0.0f);
				DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + FVector(0, 0, SWE / 10) + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);
			}
		}
	}
	const auto Location = World->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	auto PlayerController = World->GetFirstPlayerController();
	auto Pawn = PlayerController->GetPawn();

	// Render snow water equivalent
	int Index = 0;
	for (auto& Cell : Cells)
	{
		auto Offset = Cell.Normal;
		Offset.Normalize();

		// @TODO get position from heightmap
		Offset *= 10;

		if (FVector::Dist(Cell.Centroid + Offset, Location) < CellDebugInfoDisplayDistance)
		{
			FCollisionQueryParams TraceParams(FName(TEXT("Trace SWE")), true);
			TraceParams.bTraceComplex = true;
			TraceParams.AddIgnoredActor(Pawn);

			//Re-initialize hit info
			FHitResult HitOut(ForceInit);

			World->LineTraceSingleByChannel(HitOut, Location, Cell.P1 + Offset, ECC_WorldStatic, TraceParams);

			auto Hit = HitOut.GetActor();

			//Hit any Actor?
			if (Hit == NULL)
			{
				switch (DebugVisualizationType)
				{
				case EDebugVisualizationType::SWE:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.SnowWaterEquivalent)), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::SnowHeight:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.GetSnowHeight())) + " mm", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Position:
					DrawDebugString(World, Cell.Centroid, "(" + FString::FromInt(static_cast<int>(Cell.Centroid.X / 100)) + "/" + FString::FromInt(static_cast<int>(Cell.Centroid.Y / 100)) + ")", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Altitude:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.Altitude / 100)) + "m", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Index:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(Index), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Curvature:
					DrawDebugString(World, Cell.Centroid, FString::SanitizeFloat(Cell.Curvature), nullptr, FColor::Purple, 0, true);
					break;
				default:
					break;
				}
			}
		}

		Index++;
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
			auto NumLandscapes = Landscape->LandscapeComponents.Num();
			auto LastLandscapeComponent = Landscape->LandscapeComponents[NumLandscapes - 1];
			int32 NumComponentsX = LastLandscapeComponent->SectionBaseX / LastLandscapeComponent->ComponentSizeQuads + 1;
			int32 NumComponentsY = LastLandscapeComponent->SectionBaseY / LastLandscapeComponent->ComponentSizeQuads + 1;

			OverallResolutionX = Landscape->SubsectionSizeQuads * Landscape->NumSubsections * NumComponentsX + 1;
			OverallResolutionY = Landscape->SubsectionSizeQuads * Landscape->NumSubsections * NumComponentsY + 1;

			CellsDimensionX = OverallResolutionX / CellSize - 1; // -1 because we create cells and use 4 vertices
			CellsDimensionY = OverallResolutionY / CellSize - 1; // -1 because we create cells and use 4 vertices
			NumCells = CellsDimensionX * CellsDimensionY;

			UE_LOG(SimulationLog, Display, TEXT("Num components: %d"), LandscapeComponents.Num());
			UE_LOG(SimulationLog, Display, TEXT("Num subsections: %d"), Landscape->NumSubsections);
			UE_LOG(SimulationLog, Display, TEXT("SubsectionSizeQuads: %d"), Landscape->SubsectionSizeQuads);
			UE_LOG(SimulationLog, Display, TEXT("ComponentSizeQuads: %d"), Landscape->ComponentSizeQuads);
		}

		// Update shader
		SetScalarParameterValue(Landscape, TEXT("CellsDimensionX"), CellsDimensionX);
		SetScalarParameterValue(Landscape, TEXT("CellsDimensionY"), CellsDimensionY);
		SetScalarParameterValue(Landscape, TEXT("ResolutionX"), OverallResolutionX);
		SetScalarParameterValue(Landscape, TEXT("ResolutionY"), OverallResolutionY);
	}
}

void ASnowSimulationActor::UpdateMaterialTexture()
{
	auto SnowMapTexture = Simulation->GetSnowMapTexture();
	if (WriteDebugTextures)
	{
		// @TODO buggy for GPU version
		auto SnowMapPath = DebugTexturePath + "\\SnowMap";
		FFileHelper::CreateBitmap(*SnowMapPath, CellsDimensionX, CellsDimensionY, Simulation->GetSnowMapTextureData().GetData());
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

