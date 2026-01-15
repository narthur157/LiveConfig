// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LiveConfigSample : ModuleRules
{
	public LiveConfigSample(ReadOnlyTargetRules Target) : base(Target)
	{
		bUseUnity = false;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
