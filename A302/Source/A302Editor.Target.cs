// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class A302EditorTarget : TargetRules
{
	public A302EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("A302Shared");
		ExtraModuleNames.Add("A302");
		ExtraModuleNames.Add("A302Client");
		ExtraModuleNames.Add("A302Server");
		
	}
}
