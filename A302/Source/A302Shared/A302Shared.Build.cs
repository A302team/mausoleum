using UnrealBuildTool;

public class A302Shared : ModuleRules
{
	public A302Shared(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.AddRange(new string[] { "A302Shared" });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"WebSockets",
			"UMG",
			"SlateCore",
			"Json",
			"JsonUtilities",
			"Niagara"
		});
	}
}
