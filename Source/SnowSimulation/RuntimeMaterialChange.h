#pragma once
#include "LandscapeProxy.h"
#include "Materials/MaterialInstance.h"
#include "Engine/Engine.h"
#include "Private/Materials/MaterialInstanceSupport.h"

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
void SetTextureParameterValue(ALandscapeProxy* Landscape, FName ParameterName, UTexture* Value, UEngine* GEngine)
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
