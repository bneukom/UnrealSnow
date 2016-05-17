#pragma once
#include "Factories/Factory.h"
#include "BILDataFactory.generated.h"


/**
* Implements a factory for WorldClimData objects.
*/
UCLASS(hidecategories = Object)
class UBILDataFactory
	: public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	// UFactory Interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	virtual bool ConfigureProperties() override;
};