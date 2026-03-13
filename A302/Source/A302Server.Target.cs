using UnrealBuildTool;

public class A302ServerTarget : TargetRules
{
	public A302ServerTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("A302");
		ExtraModuleNames.Add("A302Server");
	}
}