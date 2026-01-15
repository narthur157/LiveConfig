// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LiveConfigSampleServerTarget : TargetRules
{
	public LiveConfigSampleServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Error;
		ExtraModuleNames.Add("LiveConfigSample");
	}
}
