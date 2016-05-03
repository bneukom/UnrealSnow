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
#include "UObject/NameTypes.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Public/RenderingThread.h"
#include "Runtime/Engine/Private/Materials/MaterialInstanceSupport.h"



DEFINE_LOG_CATEGORY(SimulationLog);

#define WRITE_SNOWMAP_TO_DISK 1

/**
* Start of code taken from MaterialInstance.cpp
*/
ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_DECLARE_TEMPLATE(
	SetMIParameterValue, ParameterType,
	const UMaterialInstance*, Instance, Instance,
	FName, ParameterName, Parameter.ParameterName,
	typename ParameterType::ValueType, Value, ParameterType::GetValue(Parameter),
	{
		Instance->Resources[0]->RenderThread_UpdateParameter(ParameterName, Value);
if (Instance->Resources[1])
{
	Instance->Resources[1]->RenderThread_UpdateParameter(ParameterName, Value);
}
if (Instance->Resources[2])
{
	Instance->Resources[2]->RenderThread_UpdateParameter(ParameterName, Value);
}
	});

/**
* Updates a parameter on the material instance from the game thread.
*/
template <typename ParameterType>
void GameThread_UpdateMIParameter(const UMaterialInstance* Instance, const ParameterType& Parameter)
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER_CREATE_TEMPLATE(
		SetMIParameterValue, ParameterType,
		const UMaterialInstance*, Instance,
		FName, Parameter.ParameterName,
		typename ParameterType::ValueType, ParameterType::GetValue(Parameter)
		);
}

/**
* Cache uniform expressions for the given material.
*  @param MaterialInstance - The material instance for which to cache uniform expressions.
*/
void CacheMaterialInstanceUniformExpressions(const UMaterialInstance* MaterialInstance)
{
	// Only cache the unselected + unhovered material instance. Selection color
	// can change at runtime and would invalidate the parameter cache.
	if (MaterialInstance->Resources[0])
	{
		MaterialInstance->Resources[0]->CacheUniformExpressions_GameThread();
	}
}

void SetVectorParameterValue(ALandscapeProxy* Landscape, FName ParameterName, FLinearColor Value)
{
	if (Landscape)
	{
		for (int32 Index = 0; Index < Landscape->LandscapeComponents.Num(); ++Index)
		{
			if (Landscape->LandscapeComponents[Index])
			{
				UMaterialInstanceConstant* MIC = Landscape->LandscapeComponents[Index]->MaterialInstance;
				if (MIC)
				{
					/**
					* Start of code taken from UMaterialInstance::SetSetVectorParameterValueInternal and adjusted to use MIC instead of this
					*/
					FVectorParameterValue* ParameterValue = GameThread_FindParameterByName( //from MaterialInstanceSupport.h
						MIC->VectorParameterValues,
						ParameterName
						);

					if (!ParameterValue)
					{
						// If there's no element for the named parameter in array yet, add one.
						ParameterValue = new(MIC->VectorParameterValues) FVectorParameterValue;
						ParameterValue->ParameterName = ParameterName;
						ParameterValue->ExpressionGUID.Invalidate();
						// Force an update on first use
						ParameterValue->ParameterValue.B = Value.B - 1.f;
					}

					// Don't enqueue an update if it isn't needed
					if (ParameterValue->ParameterValue != Value)
					{
						ParameterValue->ParameterValue = Value;
						// Update the material instance data in the rendering thread.
						GameThread_UpdateMIParameter(MIC, *ParameterValue);
						CacheMaterialInstanceUniformExpressions(MIC);
					}
					/**
					* End of code taken from UMaterialInstance::SetSetVectorParameterValueInternal and adjusted to use MIC instead of this
					*/
				}
			}
		}
	}
}
void SetTextureParameterValue(ALandscapeProxy* Landscape, FName ParameterName, UTexture* Value)
{
	if (Landscape)
	{
		for (int32 Index = 0; Index < Landscape->LandscapeComponents.Num(); ++Index)
		{
			if (Landscape->LandscapeComponents[Index])
			{
				UMaterialInstanceConstant* MIC = Landscape->LandscapeComponents[Index]->MaterialInstance;
				if (MIC)
				{
					FTextureParameterValue* ParameterValue = GameThread_FindParameterByName(
						MIC->TextureParameterValues,
						ParameterName
						);

					if (!ParameterValue)
					{
						// If there's no element for the named parameter in array yet, add one.
						ParameterValue = new(MIC->TextureParameterValues) FTextureParameterValue;
						ParameterValue->ParameterName = ParameterName;
						ParameterValue->ExpressionGUID.Invalidate();
						// Force an update on first use
						ParameterValue->ParameterValue = Value == GEngine->DefaultDiffuseTexture ? NULL : GEngine->DefaultDiffuseTexture;
					}

					// Don't enqueue an update if it isn't needed
					if (ParameterValue->ParameterValue != Value)
					{
						checkf(!Value || Value->IsA(UTexture::StaticClass()), TEXT("Expecting a UTexture! Value='%s' class='%s'"), *Value->GetName(), *Value->GetClass()->GetName());

						ParameterValue->ParameterValue = Value;
						// Update the material instance data in the rendering thread.
						GameThread_UpdateMIParameter(MIC, *ParameterValue);
						CacheMaterialInstanceUniformExpressions(MIC);
					}
				}
			}
		}
	}
}


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
		// @TODO faster method for this?
		for (int32 Y = 0; Y < CellsDimension; ++Y)
		{
			for (int32 X = 0; X < CellsDimension; ++X)
			{
				// @TODO how big should the SWE be for it to be visible
				if (Cells[Y * CellsDimension + X].SnowWaterEquivalent > 1)
				{
					SnowMaskData.Add(FColor(255, 255, 255));
				}
				else 
				{
					SnowMaskData.Add(FColor(0, 0, 0));
				}
			}
		}

		if (WriteSnowMap)
		{
			FFileHelper::CreateBitmap(*SnowMapPath, CellsDimension, CellsDimension, SnowMaskData.GetData());
		}

		FRenderCommandFence UpdateTextureFence;

		UpdateTextureFence.BeginFence();

		FUpdateTextureRegion2D* RegionData = new FUpdateTextureRegion2D(0, 0, 0, 0, SnowMaskTexture->GetSizeX(), SnowMaskTexture->GetSizeY());
		
		// Update the texture
		SnowMaskTexture->UpdateTextureRegions(
			0, 1, 
			RegionData, SnowMaskTexture->GetSizeX() * 4, 4, 
			(uint8*)SnowMaskData.GetData(), 
			[](uint8* SrcData, const FUpdateTextureRegion2D* Regions) 
			{
				delete Regions;
			}
		);
			
		UpdateTextureFence.Wait();

		SetTextureParameterValue(Landscape, TEXT("SnowMap"), SnowMaskTexture);
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

