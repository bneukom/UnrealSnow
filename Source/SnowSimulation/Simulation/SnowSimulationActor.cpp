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

	// Initialize simulation
	Simulation->Initialize(Cells, WeatherData);
	UE_LOG(SimulationLog, Display, TEXT("Simulation type used: %s"), *Simulation->GetSimulationName());
	CurrentSimulationTime = StartTime;

	// Run simulation
	CurrentStepTime = SleepTime;
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentStepTime += DeltaTime;

	if (CurrentStepTime >= SleepTime)
	{
		CurrentStepTime = 0;

		// Simulate next step
		Simulation->Simulate(this, WeatherData, Interpolator, CurrentSimulationTime, CurrentSimulationTime + FTimespan(TimeStepHours, 0, 0), TimeStepHours);

		// Update the snow material to reflect changes from the simulation
		UpdateMaterialTexture();

		SetScalarParameterValue(Landscape, TEXT("MaxSnow"), Simulation->GetMaxSnow());
			
		CurrentSimulationTime += FTimespan(TimeStepHours, 0, 0);
	}

	// Render debug information
	if (DebugVisualizationType != EDebugVisualizationType::VE_Nothing) Simulation->RenderDebug(Cells, GetWorld(), CellDebugInfoDisplayDistance, DebugVisualizationType);
	if (RenderGrid) DoRenderGrid();

}

void ASnowSimulationActor::DoRenderGrid()
{
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
}

void ASnowSimulationActor::Initialize()
{
	// Remove old elements
	Cells.Empty();

	if (GetWorld())
	{
		ULevel* Level = GetWorld()->PersistentLevel;

		TActorIterator<ALandscape> LandscapeIterator(GetWorld());

		// Take the first available landscape
		Landscape = *LandscapeIterator;

		if (Landscape)
		{
			auto& LandscapeComponents = Landscape->LandscapeComponents;
			const int32 LandscapeSizeQuads = FMath::Sqrt(LandscapeComponents.Num() * (Landscape->ComponentSizeQuads) * (Landscape->ComponentSizeQuads));

			CellsDimension = LandscapeSizeQuads / CellSize - 1; // -1 because we create cells and use 4 vertices
			NumCells = CellsDimension * CellsDimension;

			// Create Cells
			UE_LOG(SimulationLog, Display, TEXT("Num components: %d"), LandscapeComponents.Num());
			UE_LOG(SimulationLog, Display, TEXT("Num subsections: %d"), Landscape->NumSubsections);
			UE_LOG(SimulationLog, Display, TEXT("SubsectionSizeQuads: %d"), Landscape->SubsectionSizeQuads);
			UE_LOG(SimulationLog, Display, TEXT("ComponentSizeQuads: %d"), Landscape->ComponentSizeQuads);
			UE_LOG(SimulationLog, Display, TEXT("LandscapeSizeQuads: %d"), LandscapeSizeQuads);

			// @TODO what about sections?
			// Get Vertices from all components in the landscape
			TArray<FVector> CellWorldVertices;
			CellWorldVertices.SetNumUninitialized(LandscapeSizeQuads * LandscapeSizeQuads);

			for (auto Component : LandscapeComponents)
			{
				// @TODO use runtime compatible version
				FLandscapeComponentDataInterface LandscapeData(Component);
				for (int32 Y = 0; Y < Component->ComponentSizeQuads; Y++) // not +1 because the vertices are stored twice (first and last)
				{
					for (int32 X = 0; X < Component->ComponentSizeQuads; X++) // not +1 because the vertices are stored twice (first and last)
					{
						CellWorldVertices[Component->SectionBaseX + X + LandscapeSizeQuads * Y + Component->SectionBaseY * LandscapeSizeQuads] = LandscapeData.GetWorldVertex(X, Y);
					}

				}
				// @TODO insert vertices at the very end which are currently not added because we are only iterating over Quads
			}
				
			// Create Cells
			int Index = 0;
			for (int32 Y = 0; Y < CellsDimension; Y++) 
			{
				for (int32 X = 0; X < CellsDimension; X++)
				{
					auto VertexX = X * CellSize;
					auto VertexY = Y * CellSize;
					FVector P0 = CellWorldVertices[VertexY * LandscapeSizeQuads + VertexX];
					FVector P1 = CellWorldVertices[VertexY * LandscapeSizeQuads + (VertexX + CellSize)];
					FVector P2 = CellWorldVertices[(VertexY + CellSize) * LandscapeSizeQuads + VertexX];
					FVector P3 = CellWorldVertices[(VertexY + CellSize) * LandscapeSizeQuads + (VertexX + CellSize)];

					FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
					FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

					float Altitude = Centroid.Z;
					float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

					float AreaXY = FMath::Abs(FVector2D::CrossProduct(FVector2D(P0 - P3), FVector2D(P1 - P3)) / 2
											+ FVector2D::CrossProduct(FVector2D(P2 - P3), FVector2D(P0 - P3)) / 2);

					FVector P0toP3 = P3 - P0;
					FVector P0toP3ProjXY = FVector(P0toP3.X, P0toP3.Y, 0);
					float Inclination =  IsAlmostZero(P0toP3.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(P0toP3, P0toP3ProjXY) / (P0toP3.Size() * P0toP3ProjXY.Size())));
						
					// @TODO assume constant for the moment, later handle in input data
					const float Latitude = FMath::DegreesToRadians(47);

					// @TODO what is the aspect of the XY plane?
					FVector NormalProjXY = FVector(Normal.X, Normal.Y, 0);
					float Aspect = IsAlmostZero(NormalProjXY.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(North, NormalProjXY) / NormalProjXY.Size()));
					FSimulationCell Cell(Index, P0, P1, P2, P3, Normal, Area, AreaXY, Centroid, Altitude, Aspect, Inclination, Latitude);

					Cells.Add(Cell);
					Index++;
				}
			}

			// Set neighbors
			for (int32 CellIndex = 0; CellIndex < NumCells; ++CellIndex)
			{
				auto& Current = Cells[CellIndex];

				Current.Neighbours[0] = GetCellChecked(CellIndex - CellsDimension);				// N
				if ((CellIndex + 1) % CellsDimension != 0) 
					Current.Neighbours[1] = GetCellChecked(CellIndex - CellsDimension + 1);		// NE
				if ((CellIndex + 1) % CellsDimension != 0)
					Current.Neighbours[2] = GetCellChecked(CellIndex + 1);						// E
				if ((CellIndex + 1) % CellsDimension != 0)
					Current.Neighbours[3] = GetCellChecked(CellIndex + CellsDimension + 1);		// SE
					
				Current.Neighbours[4] = GetCellChecked(CellIndex + CellsDimension);				// S
				if ((CellIndex) % CellsDimension != 0)
					Current.Neighbours[5] = GetCellChecked(CellIndex + CellsDimension - 1); 	// SW
				if ((CellIndex) % CellsDimension != 0)
					Current.Neighbours[6] = GetCellChecked(CellIndex - 1);						// N
				if ((CellIndex) % CellsDimension != 0)
					Current.Neighbours[7] = GetCellChecked(CellIndex - CellsDimension - 1);		// NW
			}

			// Set inclination map
			InclinationTexture = UTexture2D::CreateTransient(CellsDimension, CellsDimension, EPixelFormat::PF_B8G8R8A8);
			InclinationTexture->UpdateResource();
			InclinationTextureData.Empty(NumCells);

			auto MaxInclination = 0.0f;
			for (int32 Y = 0; Y < CellsDimension; ++Y)
			{
				for (int32 X = 0; X < CellsDimension; ++X)
				{
					auto& Cell = Cells[Y * CellsDimension + X];
					MaxInclination = FMath::Max(MaxInclination, Cell.Inclination);

					auto Inclination = Cell.Inclination / (PI / 2) * 255;
					auto Gray = static_cast<uint8>(Inclination);
					InclinationTextureData.Add(FColor(Gray, Gray, Gray));

				}
			}

			// Debug inclination texture output
			if (WriteTextureMaps)
			{
				auto InclinationMapPath = DebugTexturePath + "\\InclinationMap";
				FFileHelper::CreateBitmap(*InclinationMapPath, CellsDimension, CellsDimension, InclinationTextureData.GetData());
			}

			// Update material
			FRenderCommandFence UpdateTextureFence;

			UpdateTextureFence.BeginFence();
			UpdateTexture(InclinationTexture, InclinationTextureData);
			UpdateTextureFence.Wait();

			SetTextureParameterValue(Landscape, TEXT("InclinationMap"), SnowMapTexture, GEngine);

			int32 OverallResolution = Landscape->SubsectionSizeQuads * FMath::Sqrt(LandscapeComponents.Num());
			SetScalarParameterValue(Landscape, TEXT("CellsDimension"), CellsDimension);
			SetScalarParameterValue(Landscape, TEXT("Resolution"), OverallResolution);
		}

		UE_LOG(SimulationLog, Display, TEXT("Num Cells: %d"), Cells.Num());

	}
}

void ASnowSimulationActor::UpdateMaterialTexture()
{
	// @TODO what about garbage collection and concurrency when creating this texture?
	// @TODO always create new texture too slow?

	// Create new textures
	SnowMapTexture = UTexture2D::CreateTransient(CellsDimension, CellsDimension, EPixelFormat::PF_B8G8R8A8);
	SnowMapTexture->UpdateResource();
	SnowMapTextureData.Empty(NumCells);

	// Update snow map texture
	for (int32 Y = 0; Y < CellsDimension; ++Y)
	{
		for (int32 X = 0; X < CellsDimension; ++X)
		{
			auto& Cell = Cells[Y * CellsDimension + X];

			// Snow map texture
			float AreaSquareMeters = Cell.Area / (100 * 100);
			float SnowMM = Cell.SnowWaterEquivalent / AreaSquareMeters;
			float Gray = SnowMM / Simulation->GetMaxSnow() * 255;
			uint8 GrayInt = static_cast<uint8>(Gray);
			SnowMapTextureData.Add(FColor(GrayInt, GrayInt, GrayInt));
		}
	}

	// Debug snow texture output
	if (WriteTextureMaps)
	{
		auto SnowMapPath = DebugTexturePath + "\\SnowMap";
		FFileHelper::CreateBitmap(*SnowMapPath, CellsDimension, CellsDimension, SnowMapTextureData.GetData());
	}

	// Update material
	FRenderCommandFence UpdateTextureFence;

	UpdateTextureFence.BeginFence();

	UpdateTexture(SnowMapTexture, SnowMapTextureData);
		
	UpdateTextureFence.Wait();

	SetTextureParameterValue(Landscape, TEXT("SnowMap"), SnowMapTexture, GEngine);
}

void ASnowSimulationActor::UpdateTexture(UTexture2D* Texture, TArray<FColor>& TextureData)
{
	FUpdateTextureRegion2D* RegionData = new FUpdateTextureRegion2D(0, 0, 0, 0, Texture->GetSizeX(), Texture->GetSizeY());

	auto CleanupFunction = [](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
	{
		delete Regions;
	};

	// Update the texture
	Texture->UpdateTextureRegions(
		0, 1,
		RegionData, Texture->GetSizeX() * 4, 4,
		(uint8*)TextureData.GetData(),
		CleanupFunction
	);
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

