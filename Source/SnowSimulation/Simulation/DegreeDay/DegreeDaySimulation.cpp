
#include "SnowSimulation.h"
#include "DegreeDaySimulation.h"
#include "SnowSimulation/Simulation/SnowSimulationActor.h"
#include "SnowSimulation/Util/MathUtil.h"
#include "SnowSimulation/Simulation/Interpolation/SimulationDataInterpolatorBase.h"

float UDegreeDaySimulation::GetMaxSnow()
{
	return MaxSnow;
}

#if SIMULATION_DEBUG
void UDegreeDaySimulation::RenderDebug(TArray<FSimulationCell>& Cells, UWorld* World, int CellDebugInfoDisplayDistance, EDebugVisualizationType DebugVisualizationType)
{
	// Draw SWE normal
	if (DebugVisualizationType == EDebugVisualizationType::SnowHeight)
	{
		for (auto& Cell : Cells)
		{
			if (Cell.SnowWaterEquivalent > 0)
			{
				FVector Normal(Cell.Normal);
				Normal.Normalize();

				// @TODO get exact position using the height map
				FVector zOffset(0, 0, 50);

				// Height of the snow for this cell
				auto AreaSquareMeters = Cell.Area / (100 * 100);
				auto SWE = Cell.SnowWaterEquivalent / AreaSquareMeters; // l/m^2 or mm
																		//DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + zOffset + (Normal * SWE * 10), FColor(255, 0, 0), false, -1, 0, 0.0f);
				DrawDebugLine(World, Cell.Centroid + zOffset, Cell.Centroid + FVector(0, 0, SWE / 10) + zOffset, FColor(255, 0, 0), false, -1, 0, 0.0f);
			}
		}
	}
	const auto Location = World->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	auto PlayerController = World->GetFirstPlayerController();
	auto Pawn = PlayerController->GetPawn();

	// Render snow water equivalent
	int Index = 0;
	for (auto& Cell : Cells)
	{
		auto Offset = Cell.Normal;
		Offset.Normalize();

		// @TODO get position from heightmap
		Offset *= 10;

		if (FVector::Dist(Cell.Centroid + Offset, Location) < CellDebugInfoDisplayDistance)
		{
			FCollisionQueryParams TraceParams(FName(TEXT("Trace SWE")), true);
			TraceParams.bTraceComplex = true;
			TraceParams.AddIgnoredActor(Pawn);

			//Re-initialize hit info
			FHitResult HitOut(ForceInit);

			World->LineTraceSingleByChannel(HitOut, Location, Cell.P1 + Offset, ECC_WorldStatic, TraceParams);

			auto Hit = HitOut.GetActor();

			//Hit any Actor?
			if (Hit == NULL)
			{
				switch (DebugVisualizationType)
				{
				case EDebugVisualizationType::SWE:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.SnowWaterEquivalent)), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::SnowHeight:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.GetSnowHeight())) + " mm", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Position:
					DrawDebugString(World, Cell.Centroid, "(" + FString::FromInt(static_cast<int>(Cell.Centroid.X / 100)) + "/" + FString::FromInt(static_cast<int>(Cell.Centroid.Y / 100)) + ")", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Altitude:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(static_cast<int>(Cell.Altitude / 100)) + "m", nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Index:
					DrawDebugString(World, Cell.Centroid, FString::FromInt(Index), nullptr, FColor::Purple, 0, true);
					break;
				case EDebugVisualizationType::Curvature:
					DrawDebugString(World, Cell.Centroid, FString::SanitizeFloat(Cell.Curvature), nullptr, FColor::Purple, 0, true);
					break;
				default:
					break;
				}
			}
		}

		Index++;
	}
}

#endif // SIMULATION_DEBUG