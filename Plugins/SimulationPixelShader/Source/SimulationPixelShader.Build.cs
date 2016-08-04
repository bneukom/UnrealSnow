namespace UnrealBuildTool.Rules
{
	public class SimulationPixelShader : ModuleRules
	{
		public SimulationPixelShader(TargetInfo Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
					"SimulationPixelShader/Private",
                    "ShaderUtility/Public"
				}
                );

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "RenderCore",
                    "ShaderCore",
                    "RHI",
                    "ShaderUtility"
				}
				);

		}
	}
}