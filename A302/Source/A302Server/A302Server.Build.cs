using UnrealBuildTool;

public class A302Server : ModuleRules
{
	public A302Server(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.AddRange(new string[] { "A302Server" });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"A302Shared"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities"
		});
	}
}
