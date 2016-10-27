#include "../PixelShaderPrivatePCH.h"
#include "AlbedoPixelShaderDeclaration.h"
#include "AlbedoPixelShader.h"
#include "RHIStaticStates.h"


#define WRITE_SNOW_MAP false

//It seems to be the convention to expose all vertex declarations as globals, and then reference them as externs in the headers where they are needed.
//It kind of makes sense since they do not contain any parameters that change and are purely used as their names suggest, as declarations :)
TGlobalResource<FTextureVertexDeclaration> GAlbedoTextureVertexDeclaration;

FAlbedoPixelShader::FAlbedoPixelShader(ERHIFeatureLevel::Type ShaderFeatureLevel)
{
	FeatureLevel = ShaderFeatureLevel;

	ConstantParameters = FAlbedoPixelShaderConstantParameters();
	
	VariableParameters = FAlbedoPixelShaderVariableParameters();
	
	bMustRegenerateSRV = false;
	bIsPixelShaderExecuting = false;
	bIsUnloading = false;
	bSave = false;

	CurrentTexture = NULL;
	CurrentRenderTarget = NULL;
}

void FAlbedoPixelShader::Initialize(FRWStructuredBuffer* AlbedoBuffer, int32 CellsDimensionX, int32 CellsDimensionY)
{
	this->AlbedoInputBuffer = AlbedoBuffer;

	ConstantParameters.CellsDimensionX = CellsDimensionX;
	ConstantParameters.CellsDimensionY = CellsDimensionY;
}

FAlbedoPixelShader::~FAlbedoPixelShader()
{
	bIsUnloading = true;
}

void FAlbedoPixelShader::ExecutePixelShader(UTextureRenderTarget2D* RenderTarget, bool SaveAlbedo)
{
	if (bIsUnloading || bIsPixelShaderExecuting) //Skip this execution round if we are already executing
	{
		return;
	}

	bIsPixelShaderExecuting = true;

	CurrentRenderTarget = RenderTarget;

	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
		FPixelShaderRunner,
		FAlbedoPixelShader*, PixelShader, this,
		bool, SaveAlbedo, SaveAlbedo,
		{
			PixelShader->ExecutePixelShaderInternal(SaveAlbedo);
		}
	);
}

void FAlbedoPixelShader::ExecutePixelShaderInternal(bool SaveAlbedo)
{
	check(IsInRenderingThread());

	// Only cleanup
	if (bIsUnloading) 
	{
		return;
	}

	FRHICommandListImmediate& RHICmdList = GRHICommandList.GetImmediateCommandList();

	CurrentTexture = CurrentRenderTarget->GetRenderTargetResource()->GetRenderTargetTexture();
	SetRenderTarget(RHICmdList, CurrentTexture, FTextureRHIRef());
	RHICmdList.SetBlendState(TStaticBlendState<>::GetRHI());
	RHICmdList.SetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHICmdList.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
	
	static FGlobalBoundShaderState BoundShaderState;
	TShaderMapRef<FAlbedoVertexShader> VertexShader(GetGlobalShaderMap(FeatureLevel));
	TShaderMapRef<FAlbedoPixelShaderDeclaration> PixelShader(GetGlobalShaderMap(FeatureLevel));

	SetGlobalBoundShaderState(RHICmdList, FeatureLevel, BoundShaderState, GAlbedoTextureVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters(RHICmdList, AlbedoInputBuffer->SRV);
	PixelShader->SetUniformBuffers(RHICmdList, ConstantParameters, VariableParameters);

	// Draw a fullscreen quad that we can run our pixel shader on
	FTextureVertex Vertices[4];
	Vertices[0].Position = FVector4(-1.0f, 1.0f, 0, 1.0f);
	Vertices[1].Position = FVector4(1.0f, 1.0f, 0, 1.0f);
	Vertices[2].Position = FVector4(-1.0f, -1.0f, 0, 1.0f);
	Vertices[3].Position = FVector4(1.0f, -1.0f, 0, 1.0f);
	Vertices[0].UV = FVector2D(0, 0);
	Vertices[1].UV = FVector2D(ConstantParameters.CellsDimensionX, 0);
	Vertices[2].UV = FVector2D(0, ConstantParameters.CellsDimensionY);
	Vertices[3].UV = FVector2D(ConstantParameters.CellsDimensionX, ConstantParameters.CellsDimensionY);

	DrawPrimitiveUP(RHICmdList, PT_TriangleStrip, 2, Vertices, sizeof(Vertices[0]));
	 
	bIsPixelShaderExecuting = false;
	
	if (SaveAlbedo)
	{
		TArray<FColor> Bitmap;

		FReadSurfaceDataFlags ReadDataFlags;
		ReadDataFlags.SetLinearToGamma(false);
		ReadDataFlags.SetOutputStencil(false);
		ReadDataFlags.SetMip(0);

		//This is pretty straight forward. Since we are using a standard format, we can use this convenience function instead of having to lock rect.
		RHICmdList.ReadSurfaceData(CurrentTexture, FIntRect(0, 0, CurrentTexture->GetSizeX(), CurrentTexture->GetSizeY()), Bitmap, ReadDataFlags);

		// if the format and texture type is supported
		if (Bitmap.Num())
		{
			// Create screenshot folder if not already present.
			IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true);

			const FString ScreenFileName(FPaths::ScreenShotDir() / TEXT("SnowMap"));

			uint32 ExtendXWithMSAA = Bitmap.Num() / CurrentTexture->GetSizeY();

			// Save the contents of the array to a bitmap file. (24bit only so alpha channel is dropped)
			FFileHelper::CreateBitmap(*ScreenFileName, ExtendXWithMSAA, CurrentTexture->GetSizeY(), Bitmap.GetData());

		}
		else
		{
			UE_LOG(LogConsoleResponse, Error, TEXT("Failed to save BMP, format or texture type is not supported"));
		}
	}
}



