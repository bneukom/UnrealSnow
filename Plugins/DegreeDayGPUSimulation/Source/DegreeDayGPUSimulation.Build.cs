namespace UnrealBuildTool.Rules
{
	public class DegreeDayGPUSimulation : ModuleRules
	{
		public DegreeDayGPUSimulation(TargetInfo Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
					"DegreeDayGPUSimulation/Private"
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
                    "RHI"
				}
				);
		}
	}
}