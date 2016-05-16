namespace UnrealBuildTool.Rules
{
    public class UObjectPlugin : ModuleRules
    {
        public UObjectPlugin(TargetInfo Target)
        {
            PublicIncludePaths.AddRange(
                new string[] {
                    "Developer/UObjectPlugin/Source/UObjectPlugin/Public",
					// ... add public include paths required here ...
				}
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "Developer/UObjectPlugin/Source/UObjectPlugin/Private",
					// ... add other private include paths required here ...
				}
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
					// ... add other public dependencies that you statically link with here ...
				}
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
					// ... add private dependencies that you statically link with here ...
				}
                );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
					// ... add any modules that your module loads dynamically here ...
				}
                );
        }
    }
}