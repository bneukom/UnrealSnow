namespace UnrealBuildTool.Rules
{
    public class WorldClimData : ModuleRules
    {
        public WorldClimData(TargetInfo Target)
        {
            PublicIncludePaths.AddRange(
                new string[] {
                    "WorldClimData/Public",
				}
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "UnrealEd",
                    "WorldClimData/Private",
                    "WorldClimData/Classes",
                }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "AssetTools",
                    "UnrealEd",
                    "EditorStyle"
				}
                );

        }
    }
}