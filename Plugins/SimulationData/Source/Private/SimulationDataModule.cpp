// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "Public/SimulationData.h"

class SimulationDataModule : public IModuleInterface
{
public:
	virtual void SimulationDataModule::StartupModule() override
	{

	}


	virtual void SimulationDataModule::ShutdownModule()
	{

	}
};

IMPLEMENT_MODULE(SimulationDataModule, SimulationData)