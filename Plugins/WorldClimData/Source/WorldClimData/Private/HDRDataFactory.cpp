#include "WorldClimDataPrivatePCH.h"
#include "HDRDataFactory.h"
#include "Public/HDRData.h"

UHDRDataFactory::UHDRDataFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UHDRData::StaticClass();

	bCreateNew = false;
	bEditorImport = true;

	Formats.Add("hdr;Header file for *.bil files.");
}

UObject* UHDRDataFactory::FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn)
{
	auto Content = FString(UTF8_TO_TCHAR(Buffer));

	TArray<FString> LineBuffer;
	Content.ParseIntoArrayLines(LineBuffer);

	TMap<FString, FString> DataMap;
	for (FString& Line : LineBuffer)
	{
		TArray<FString> DataBuffer;
		Line.ParseIntoArray(DataBuffer, TEXT(" "));

		if (DataBuffer.Num() == 2)
		{
			DataMap.Add(DataBuffer[0], DataBuffer[1]);
		}
	}

	UHDRData* Data = NewObject<UHDRData>(InParent, SupportedClass, Name, Flags | RF_Transactional);
	
	Data->ULXMAP = FCString::Atof(*DataMap["ULXMAP"]);
	Data->ULYMAP = FCString::Atof(*DataMap["ULYMAP"]);
	Data->XDIM = FCString::Atof(*DataMap["XDIM"]);
	Data->YDIM = FCString::Atof(*DataMap["YDIM"]);
	Data->MinValue = FCString::Atoi(*DataMap["MinValue"]);
	Data->MaxValue = FCString::Atoi(*DataMap["MaxValue"]);
	Data->Month = DataMap["Month"];
	Data->NROWS = FCString::Atoi(*DataMap["NROWS"]);
	Data->NCOLS = FCString::Atoi(*DataMap["NCOLS"]);
	Data->NBANDS = FCString::Atoi(*DataMap["NBANDS"]);
	Data->NBITS = FCString::Atoi(*DataMap["NBITS"]);

	// @TODO rest of the data

	return Data;
}

bool UHDRDataFactory::FactoryCanImport(const FString& Filename)
{
	return true;
}

bool UHDRDataFactory::ConfigureProperties()
{
	return true;
}
