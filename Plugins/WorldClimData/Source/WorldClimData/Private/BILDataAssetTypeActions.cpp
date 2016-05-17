
#include "WorldClimDataPrivatePCH.h"
#include "UnrealEd.h"
#include "Classes/BILData.h"
#include "Classes/BILDataAssetTypeActions.h"

FText FBILDataAssetTypeActions::GetName() const
{
	return FText::FromString("BIL Data Asset");
}
FColor FBILDataAssetTypeActions::GetTypeColor() const
{
	return FColor(255, 0, 0);
}
UClass* FBILDataAssetTypeActions::GetSupportedClass() const
{
	return UBILData::StaticClass();
}
void FBILDataAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
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
uint32 FBILDataAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}