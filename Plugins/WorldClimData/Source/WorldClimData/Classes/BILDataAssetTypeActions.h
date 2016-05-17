#pragma once

#include "UnrealEd.h"
#include "AssetTypeActions_Base.h"


class FBILDataAssetTypeActions : public FAssetTypeActions_Base
{
public:

	// IAssetTypeActions interface
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	//virtual FText GetAssetDescription(const FAssetData& AssetData) const override;
	// End of IAssetTypeActions interface
};