#include "SnowSimulation.h"
#include "UnrealMathUtility.h"
#include "SnowSimulationActor.h"
#include "MathUtil.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RenderCommandFence.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeComponent.h"
#include "Runtime/Landscape/Classes/LandscapeInfo.h"
#include "Runtime/Landscape/Classes/LandscapeProxy.h"
#include "Runtime/Landscape/Public/LandscapeDataAccess.h" //#include "RuntimeLandscapeDataAccess.h"




DEFINE_LOG_CATEGORY(SimulationLog);

#define WRITE_SNOWMAP_TO_DISK 1

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
	Simulation->Initialize(Cells, Data);
	UE_LOG(SimulationLog, Display, TEXT("Simulation type used: %s"), *Simulation->GetSimulationName());
	CurrentSimulationTime = StartTime;
}

void ASnowSimulationActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	NextStep += DeltaTime;

	if (NextStep >= SleepTime)
	{
		NextStep = 0;

		// Simulate next step
		Simulation->Simulate(Cells, Data, Interpolator, CurrentSimulationTime, CurrentSimulationTime + SimulationStep);

		// Update the snow material to reflect changes from the simulation
		UpdateMaterialTexture();
			
		CurrentSimulationTime += SimulationStep;
	}

	// Render debug information
	if (RenderDebugInfo)
	{
		Simulation->RenderDebug(Cells, GetWorld());
	}
	
	if (RenderGrid) 
	{
		for (FSimulationCell& Cell : Cells)
		{
			FVector Normal(Cell.Normal);
			Normal.Normalize();

			// @TODO get exact position using the height map
			FVector zOffset(0, 0, 50);

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
				
			// Initialize snow mask texture
			SnowMaskTexture = UTexture2D::CreateTransient(CellsDimension, CellsDimension, EPixelFormat::PF_B8G8R8A8);
			SnowMaskTexture->UpdateResource();
			SnowMaskData.Empty(NumCells);

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
		}

		UE_LOG(SimulationLog, Display, TEXT("Num Cells: %d"), Cells.Num());

	}
}

void ASnowSimulationActor::UpdateMaterialTexture()
{
	// @TODO what about garbage collection and concurrency when creating this texture?
	// Update Texture Data
	SnowMaskTexture = UTexture2D::CreateTransient(CellsDimension, CellsDimension, EPixelFormat::PF_B8G8R8A8);
	SnowMaskTexture->UpdateResource();
	SnowMaskData.Empty(NumCells);

	if (SnowMaskTexture->Resource)
	{
		struct FUpdateTextureData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			FUpdateTextureRegion2D Region;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
			ASnowSimulationActor* SimulationActor;
		};

		for (int32 Y = 0; Y < CellsDimension; ++Y)
		{
			for (int32 X = 0; X < CellsDimension; ++X)
			{
				if (Cells[Y * CellsDimension + X].SnowWaterEquivalent > 0)
				{
					SnowMaskData.Add(FLinearColor(1, 1, 1));
				}
				else 
				{
					SnowMaskData.Add(FLinearColor(0, 0, 0));
				}
			}
		}

		if (WriteSnowMap)
		{
			TArray<FColor> ColorArray;
			for (int32 Index = 0; Index < SnowMaskData.Num(); ++Index)
			{
				int32 R = static_cast<int32>(SnowMaskData[Index].R * 255);
				int32 G = static_cast<int32>(SnowMaskData[Index].G * 255);
				int32 B = static_cast<int32>(SnowMaskData[Index].B * 255);
				ColorArray.Add(FColor(R, G, B));
			}

			FFileHelper::CreateBitmap(*SnowMapPath, CellsDimension, CellsDimension, ColorArray.GetData());
		
		}

		FRenderCommandFence UpdateTextureFence;
		FUpdateTextureData* UpdateTextureData = new FUpdateTextureData;

		UpdateTextureData->Texture2DResource = (FTexture2DResource*)SnowMaskTexture->Resource;
		UpdateTextureData->MipIndex = 0;
		UpdateTextureData->Region = FUpdateTextureRegion2D(0, 0, 0, 0, SnowMaskTexture->GetSizeX(), SnowMaskTexture->GetSizeY());
		UpdateTextureData->SrcPitch = SnowMaskTexture->GetSizeX() * sizeof(FLinearColor);
		UpdateTextureData->SrcBpp = sizeof(FLinearColor);
		UpdateTextureData->SrcData = (uint8*)SnowMaskData.GetData();
		UpdateTextureData->SimulationActor = this;

		UpdateTextureFence.BeginFence();
		{
			
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
				UpdateTextureRegionsData,
				FUpdateTextureData*, UpdateTextureData, UpdateTextureData,
			{
				int32 CurrentFirstMip = UpdateTextureData->Texture2DResource->GetCurrentFirstMip();
				if (UpdateTextureData->MipIndex >= CurrentFirstMip)
				{
						
					RHIUpdateTexture2D(
						UpdateTextureData->Texture2DResource->GetTexture2DRHI(),
						UpdateTextureData->MipIndex - CurrentFirstMip,
						UpdateTextureData->Region,
						UpdateTextureData->SrcPitch,
						UpdateTextureData->SrcData
						+ UpdateTextureData->Region.SrcY * UpdateTextureData->SrcPitch
						+ UpdateTextureData->Region.SrcX * UpdateTextureData->SrcBpp
					);
				}

		

				delete UpdateTextureData;
			});
			
		}

		UpdateTextureFence.Wait();

		//auto MaterialInstance = UMaterialInstanceDynamic::Create(LandscapeMaterial, this);
		//MaterialInstance->SetTextureParameterValue("SnowMap", SnowMaskTexture);

		for (auto Component : Landscape->LandscapeComponents)
		{
			auto MaterialInstance =  Component->CreateAndSetMaterialInstanceDynamic(0);
			MaterialInstance->SetTextureParameterValue("SnowMap", SnowMaskTexture);
			Component->UpdateMaterialInstances();
		}
		/*
		for (auto Component : SimulationActor->Landscape->LandscapeComponents)
		{
			Component->SetMaterial(0, MaterialInstance);
		}
		*/
		
	}
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

