#pragma once

#include "UObjectGlobals.h"
#include "SimulationHUD.generated.h"


UCLASS()
class ASimulationHUD : public AHUD
{
	GENERATED_UCLASS_BODY()

	/** Put Roboto Here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UFont* UE4Font;

public:

	void DrawHUD() override;
};