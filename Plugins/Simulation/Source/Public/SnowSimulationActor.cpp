#include "Simulation.h"
#include "UnrealMathUtility.h"
#include "SnowSimulationActor.h"
#include "Util/MathUtil.h"
#include "Util/TextureUtil.h"
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
	Simulation->Initialize(this, LandscapeCells, InitialMaxSnow, GetWorld());
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
		Simulation->Simulate(this, CurrentSimulationStep, Timesteps, SaveMaterialTextures, DebugVisualizationType != EDebugVisualizationType::Nothing, DebugCells);

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
	if (DebugVisualizationType != EDebugVisualizationType::Nothing) DoRenderDebugInformation();
	if (RenderGrid) DoRenderGrid();

}

void ASnowSimulationActor::DoRenderGrid()
{
	const auto Location = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	
	for (FDebugCell& Cell : DebugCells)
	{
		FVector Normal(Cell.Normal);
		Normal.Normalize();

		// @TODO get exact position using the height map
		FVector zOffset(0, 0, DebugGridZOffset);

		if (FVector::Dist(Cell.Centroid, Location) < CellDebugInfoDisplayDistance)
		{
			// Draw Cells
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P2 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P1 + zOffset, Cell.P3 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P2 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
			DrawDebugLine(GetWorld(), Cell.P3 + zOffset, Cell.P4 + zOffset, FColor(255, 255, 0), false, -1, 0, 0.0f);
		}
	}
}

void ASnowSimulationActor::DoRenderDebugInformation()
{
	const auto Location = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	auto PlayerController = GetWorld()->GetFirstPlayerController();
	auto Pawn = PlayerController->GetPawn();

	// Draw SWE normal
	if (DebugVisualizationType == EDebugVisualizationType::SnowHeight)
	{
		for (auto& Cell : DebugCells)
		{
			if (Cell.SnowMM > 0 && FVector::Dist(Cell.P1, Location) < CellDebugInfoDisplayDistance)
			{
				FVector Normal(Cell.Normal);
				Normal.Normalize();

				// @TODO get exact position using the height map
				FVector zOffset(0, 0, DebugGridZOffset);

				DrawDebugLine(GetWorld(), Cell.Centroid + zOffset, Cell.Centroid + FVector(0, 0, Cell.SnowMM / 10) + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);
			}
		}
	}

	// Render debug strings
	int Index = 0;
	for (auto& Cell : DebugCells)
	{
		auto Offset = Cell.Normal;
		Offset.Normalize();

		// @TODO get position from heightmap
		Offset *= 10;

		if (FVector::Dist(Cell.P1 + Offset, Location) < CellDebugInfoDisplayDistance)
		{
			FCollisionQueryParams TraceParams(FName(TEXT("Trace SWE")), true);
			TraceParams.bTraceComplex = true;
			TraceParams.AddIgnoredActor(Pawn);

			//Re-initialize hit info
			FHitResult HitOut(ForceInit);

			GetWorld()->LineTraceSingleByChannel(HitOut, Location, Cell.P1 + Offset, ECC_WorldStatic, TraceParams);

			auto Hit = HitOut.GetActor();

			//Hit any Actor?
			if (Hit == NULL)
			{
				switch (DebugVisualizationType)
				{
				case EDebugVisualizationType::SnowHeight:
					DrawDebugString(GetWorld(), Cell.Centroid, FString::FromInt(static_cast<int>(Cell.SnowMM)) + " mm", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Position:
					DrawDebugString(GetWorld(), Cell.Centroid, "(" + FString::FromInt(static_cast<int>(Cell.Centroid.X / 100)) + "/" + FString::FromInt(static_cast<int>(Cell.Centroid.Y / 100)) + ")", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Altitude:
					DrawDebugString(GetWorld(), Cell.Centroid, FString::FromInt(static_cast<int>(Cell.Altitude / 100)) + "m", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Index:
					DrawDebugString(GetWorld(), Cell.Centroid, FString::FromInt(Index), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Curvature:
					DrawDebugString(GetWorld(), Cell.Centroid, FString::SanitizeFloat(Cell.Curvature), nullptr, FColor::Purple, 0, true);
					break;
				default:
					break;
				}
			}
		}

		Index++;
	}
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

			DebugCells.Reserve(CellsDimensionX * CellsDimensionY);
			LandscapeCells.Reserve(CellsDimensionX * CellsDimensionY);

			TArray<FVector> CellWorldVertices;
			CellWorldVertices.SetNumUninitialized(OverallResolutionX * OverallResolutionY);

			float MinAltitude = 1e6;
			float MaxAltitude = 0;
			for (auto Component : LandscapeComponents)
			{
				// @TODO use runtime compatible version
				FLandscapeComponentDataInterface LandscapeData(Component);
				for (int32 Y = 0; Y < Component->ComponentSizeQuads; Y++) // not +1 because the vertices are stored twice (first and last)
				{
					for (int32 X = 0; X < Component->ComponentSizeQuads; X++) // not +1 because the vertices are stored twice (first and last)
					{
						auto Vertex = LandscapeData.GetWorldVertex(X, Y);
						CellWorldVertices[Component->SectionBaseX + X + OverallResolutionX * Y + Component->SectionBaseY * OverallResolutionX] = Vertex;
						MinAltitude = FMath::Min(MinAltitude, Vertex.Z);
						MaxAltitude = FMath::Max(MaxAltitude, Vertex.Z);
					}
				}
			}

			// Distance between neighboring cells in cm (calculate as in https://forums.unrealengine.com/showthread.php?57338-Calculating-Exact-Map-Size)
			const float L = LandscapeScale.X / 100 * CellSize;

			/*
			// Calculate slope map
			SlopeTexture = UTexture2D::CreateTransient((OverallResolutionX - 1), (OverallResolutionY - 1), EPixelFormat::PF_B8G8R8A8);
			SlopeTexture->UpdateResource();

			TArray<FColor> SlopeTextureData;
			SlopeTextureData.Empty((OverallResolutionX - 1) * (OverallResolutionY - 1));

			for (int32 Y = 1; Y < OverallResolutionY - 1; Y++)
			{
				for (int32 X = 1; X < OverallResolutionX - 1; X++)
				{
					FVector C1 = CellWorldVertices[(Y - 1) * (OverallResolutionX - 1) + (X - 1)];
					FVector C2 = CellWorldVertices[(Y - 1) * (OverallResolutionX - 1) + (X + 0)];
					FVector C3 = CellWorldVertices[(Y - 1) * (OverallResolutionX - 1) + (X + 1)];
					FVector C4 = CellWorldVertices[(Y + 0) * (OverallResolutionX - 1) + (X - 1)];
					FVector C5 = CellWorldVertices[(Y + 0) * (OverallResolutionX - 1) + (X + 0)];
					FVector C6 = CellWorldVertices[(Y + 0) * (OverallResolutionX - 1) + (X + 1)];
					FVector C7 = CellWorldVertices[(Y + 1) * (OverallResolutionX - 1) + (X - 1)];
					FVector C8 = CellWorldVertices[(Y + 1) * (OverallResolutionX - 1) + (X + 0)];
					FVector C9 = CellWorldVertices[(Y + 1) * (OverallResolutionX - 1) + (X + 1)];

					float x = (C3.Z + 2 * C6.Z + C9.Z - C1.Z - 2 * C4.Z - C7.Z) / (8 * L * 100);
					float y = (C1.Z + 2 * C2.Z + C3.Z - C7.Z - 2 * C8.Z - C9.Z) / (8 * L * 100);
					float Slope = FMath::Atan(FMath::Sqrt(x * x + y * y));
					float GrayScale = Slope / (PI / 2) * 255;
					uint8 GrayInt = static_cast<uint8>(GrayScale);
					SlopeTextureData.Add(FColor(GrayInt, GrayInt, GrayInt));
				}
			}
			FRenderCommandFence UpdateTextureFence;
			UpdateTextureFence.BeginFence();

			UpdateTexture(SlopeTexture, SlopeTextureData);

			UpdateTextureFence.Wait();

			if (SaveMaterialTextures)
			{
				// Create screenshot folder if not already present.
				IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

				const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("SlopeMap"));

				uint32 ExtendXWithMSAA = SlopeTextureData.Num() / SlopeTexture->GetSizeY();

				// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
				FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, SlopeTexture->GetSizeY(), SlopeTextureData.GetData());
			}
			*/

			// Create Cells
			
			int Index = 0;
			for (int32 Y = 0; Y < CellsDimensionY; Y++)
			{
				for (int32 X = 0; X < CellsDimensionX; X++)
				{
					auto VertexX = X * CellSize;
					auto VertexY = Y * CellSize;
					FVector P0 = CellWorldVertices[VertexY * OverallResolutionX + VertexX];
					FVector P1 = CellWorldVertices[VertexY * OverallResolutionX + (VertexX + CellSize)];
					FVector P2 = CellWorldVertices[(VertexY + CellSize) * OverallResolutionX + VertexX];
					FVector P3 = CellWorldVertices[(VertexY + CellSize) * OverallResolutionX + (VertexX + CellSize)];

					FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0);
					FVector Centroid = FVector((P0.X + P1.X + P2.X + P3.X) / 4, (P0.Y + P1.Y + P2.Y + P3.Y) / 4, (P0.Z + P1.Z + P2.Z + P3.Z) / 4);

					float Altitude = Centroid.Z;

					float Area = FMath::Abs(FVector::CrossProduct(P0 - P3, P1 - P3).Size() / 2 + FVector::CrossProduct(P2 - P3, P0 - P3).Size() / 2);

					float AreaXY = FMath::Abs(FVector2D::CrossProduct(FVector2D(P0 - P3), FVector2D(P1 - P3)) / 2
						+ FVector2D::CrossProduct(FVector2D(P2 - P3), FVector2D(P0 - P3)) / 2);

					FVector P0toP3 = P3 - P0;
					FVector P0toP3ProjXY = FVector(P0toP3.X, P0toP3.Y, 0);
					float Inclination = IsAlmostZero(P0toP3.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(P0toP3, P0toP3ProjXY) / (P0toP3.Size() * P0toP3ProjXY.Size())));

					// @TODO assume constant for the moment, later handle in input data
					const float Latitude = FMath::DegreesToRadians(47);

					// @TODO what is the aspect of the XY plane?
					FVector NormalProjXY = FVector(Normal.X, Normal.Y, 0);
					float Aspect = IsAlmostZero(NormalProjXY.Size()) ? 0 : FMath::Abs(FMath::Acos(FVector::DotProduct(North, NormalProjXY) / NormalProjXY.Size()));

					// Initial conditions
					float SnowWaterEquivalent = 0.0f;
					if (Altitude / 100.0f > 3300.0f)
					{
						auto AreaSquareMeters = Area / (100 * 100);
						float we = (2.5 + Altitude / 100 * 0.001) * AreaSquareMeters;

						SnowWaterEquivalent = we;

						InitialMaxSnow = FMath::Max(SnowWaterEquivalent / AreaSquareMeters, InitialMaxSnow);
					}

					// Create cells
					FLandscapeCell Cell(Index, P0, P1, P2, P3, Normal, Area, AreaXY, Centroid, Altitude, Aspect, Inclination, Latitude, SnowWaterEquivalent);
					LandscapeCells.Add(Cell);

					FDebugCell DebugCell(P0, P1, P2, P3, Centroid, Normal, Altitude);
					DebugCells.Add(DebugCell);

					Index++;
				}
			}

		
			// Calculate curvature
			for (int32 CellIndexY = 0; CellIndexY < CellsDimensionY; ++CellIndexY)
			{
				for (int32 CellIndexX = 0; CellIndexX < CellsDimensionX; ++CellIndexX)
				{
					FLandscapeCell& Cell = LandscapeCells[CellIndexX + CellsDimensionX * CellIndexY];
					FLandscapeCell* Neighbours[8];

					Neighbours[0] = GetCellChecked(CellIndexX, CellIndexY - 1);						// N
					Neighbours[1] = GetCellChecked(CellIndexX + 1, CellIndexY - 1);					// NE
					Neighbours[2] = GetCellChecked(CellIndexX + 1, CellIndexY);						// E
					Neighbours[3] = GetCellChecked(CellIndexX + 1, CellIndexY + 1);					// SE

					Neighbours[4] = GetCellChecked(CellIndexX, CellIndexY + 1);						// S
					Neighbours[5] = GetCellChecked(CellIndexX - 1, CellIndexY + 1); 				// SW
					Neighbours[6] = GetCellChecked(CellIndexX - 1, CellIndexY);						// W
					Neighbours[7] = GetCellChecked(CellIndexX - 1, CellIndexY - 1);					// NW

					if (Neighbours[0] == nullptr || Neighbours[1] == nullptr || Neighbours[2] == nullptr || Neighbours[3] == nullptr
						|| Neighbours[4] == nullptr || Neighbours[5] == nullptr || Neighbours[6] == nullptr || Neighbours[7] == nullptr) continue;

					float Z1 = Neighbours[1]->Altitude / 100; // NW
					float Z2 = Neighbours[0]->Altitude / 100; // N
					float Z3 = Neighbours[7]->Altitude / 100; // NE
					float Z4 = Neighbours[2]->Altitude / 100; // W
					float Z5 = Cell.Altitude / 100;
					float Z6 = Neighbours[6]->Altitude / 100; // E
					float Z7 = Neighbours[3]->Altitude / 100; // SW	
					float Z8 = Neighbours[4]->Altitude / 100; // S
					float Z9 = Neighbours[5]->Altitude / 100; // SE

					float D = ((Z4 + Z6) / 2 - Z5) / (L * L);
					float E = ((Z2 + Z8) / 2 - Z5) / (L * L);
					Cell.Curvature = 2 * (D + E);
				}
			}

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

