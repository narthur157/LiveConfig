// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LiveConfigSampleTarget : TargetRules
{
	public LiveConfigSampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Error;
		ExtraModuleNames.Add("LiveConfigSample");
	}
}
