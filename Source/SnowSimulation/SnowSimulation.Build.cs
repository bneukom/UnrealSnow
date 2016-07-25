// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class SnowSimulation : ModuleRules
{
	public SnowSimulation(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Landscape", "RHI" , "WorldClimData", "SnowSimulation", "SimplexNoise", "ComputeShader" });
    }
}
