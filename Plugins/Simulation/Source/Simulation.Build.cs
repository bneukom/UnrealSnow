namespace UnrealBuildTool.Rules
{
	public class Simulation : ModuleRules
	{
		public Simulation(TargetInfo Target)
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
                      "Core", "CoreUObject", "Engine", "InputCore", "RenderCore", "ShaderCore", "Landscape", "RHI", "WorldClimData",
                        "SimplexNoise", "ShaderUtility", "SimulationPixelShader", "SimulationData"
                }
				);
		}
	}
}