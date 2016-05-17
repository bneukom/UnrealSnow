
#include "WorldClimDataPrivatePCH.h"
#include "UnrealEd.h"
#include "Classes/HDRData.h"
#include "Classes/HDRDataAssetTypeActions.h"

FText FHDRDataAssetTypeActions::GetName() const
{
	return FText::FromString("HDR File for *.BIL");
}
FColor FHDRDataAssetTypeActions::GetTypeColor() const
{
	return FColor(255, 0, 0);
}
UClass* FHDRDataAssetTypeActions::GetSupportedClass() const
{
	return UHDRData::StaticClass();
}
void FHDRDataAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	/*
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ?
		EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UItemDataAsset* ItemData = Cast<UItemDataAsset>(*ObjIt))
		{
			TSharedRef<FARItemEditor> NewItemEditor(new FARItemEditor());
			NewItemEditor->InitiItemEditor(Mode, EditWithinLevelEditor, ItemData);
		}
	}
	*/
}
uint32 FHDRDataAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}