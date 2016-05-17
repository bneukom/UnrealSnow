#include "WorldClimDataPrivatePCH.h"
#include "BILDataFactory.h"
#include "Classes/BILData.h"

UBILDataFactory::UBILDataFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UBILData::StaticClass();

	bCreateNew = false;
	bEditorImport = true;

	Formats.Add("bil;Binary Interleaved by Line");
}

UObject* UBILDataFactory::FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	FString NewName = Name.ToString() + ".bil";
	UBILData* BILData = NewObject<UBILData>(InParent, SupportedClass, FName(*NewName), Flags | RF_Transactional);

	SIZE_T DataSize = BufferEnd - Buffer;

	BILData->Data.Reset(DataSize);
	BILData->Data.AddUninitialized(DataSize);
	FMemory::Memcpy(BILData->Data.GetData(), Buffer, DataSize);

	return BILData;
}

bool UBILDataFactory::FactoryCanImport(const FString& Filename)
{
	return true;
}

bool UBILDataFactory::ConfigureProperties()
{
	return true;
}
