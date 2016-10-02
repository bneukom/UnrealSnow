namespace UnrealBuildTool.Rules
{
	public class SimulationData : ModuleRules
	{
		public SimulationData(TargetInfo Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
					"Simulation/Private",
                    "ShaderUtility/Public"
                }
                );

            PublicDependencyModuleNames.AddRange(
				new string[]
				{
                      "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "Landscape", "RHI", "WorldClimData",
                        "SimplexNoise", "ShaderUtility", "SimulationPixelShader"
                }
				);
		}
	}
}