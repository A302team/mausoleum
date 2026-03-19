using UnrealBuildTool;

public class A302Client : ModuleRules
{
	public A302Client(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicIncludePaths.AddRange(new string[] { "A302Client" });

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"A302Shared"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"NetCore",
			"Sockets",
			"Networking",
			"Json",
			"JsonUtilities",
			"InputCore",
			"EnhancedInput",
			"SlateCore",
			"UMG",
			"Voice",
			"AudioCapture",
			"AudioMixer",
			"libOpus",
			"Niagara"
		});
	}
}
