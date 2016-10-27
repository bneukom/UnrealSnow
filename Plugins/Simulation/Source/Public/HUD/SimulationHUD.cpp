#include "Simulation.h"
#include "SimulationHUD.h"
#include "SnowSimulationActor.h"


ASimulationHUD::ASimulationHUD(const FObjectInitializer& Initializer) : Super(Initializer) {}


void ASimulationHUD::DrawHUD()
{

	TActorIterator<ASnowSimulationActor> LandscapeIterator(GetWorld());
	auto Simulation = *LandscapeIterator;

	if (Simulation->DrawDate) 
	{
		auto YearString = FString::FromInt(Simulation->CurrentSimulationTime.GetYear());
		auto MonthString = FString::Printf(TEXT("%02d"), Simulation->CurrentSimulationTime.GetMonth());
		auto DayString = FString::Printf(TEXT("%02d"), Simulation->CurrentSimulationTime.GetDay());

		FText DateText = FText::FromString(YearString + "." + MonthString + "." + DayString);
		FLinearColor DateColor = FLinearColor(1.0f, 1.0f, 1.0f);
		FCanvasTextItem TextItem(FVector2D(20, 20), DateText, UE4Font, FColor(0, 0, 0, 255));

		//Text Scale
		TextItem.Scale.Set(1.25f, 1.25f);

		//Draw
		Canvas->DrawItem(TextItem);
	}
}