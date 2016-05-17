// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "WorldClimDataPrivatePCH.h"
#include "Classes/HDRDataAssetTypeActions.h"
#include "Classes/BILDataAssetTypeActions.h"

class WorldClimDataModule : public IModuleInterface
{
private:
	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

public:
	virtual void WorldClimDataModule::StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
		TSharedRef<IAssetTypeActions> BILData = MakeShareable(new FBILDataAssetTypeActions);
		TSharedRef<IAssetTypeActions> HDRData = MakeShareable(new FHDRDataAssetTypeActions);

		AssetTools.RegisterAssetTypeActions(BILData);
		RegisteredAssetTypeActions.Add(BILData);

		AssetTools.RegisterAssetTypeActions(HDRData);
		RegisteredAssetTypeActions.Add(HDRData);
	}


	virtual void WorldClimDataModule::ShutdownModule()
	{

		FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");

		if (AssetToolsModule != nullptr)
		{
			IAssetTools& AssetTools = AssetToolsModule->Get();

			for (auto Action : RegisteredAssetTypeActions)
			{
				AssetTools.UnregisterAssetTypeActions(Action);
			}
		}
	}
};

IMPLEMENT_MODULE(WorldClimDataModule, WorldClimData)