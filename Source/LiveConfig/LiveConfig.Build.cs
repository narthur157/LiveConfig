// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

using UnrealBuildTool;

public class LiveConfig : ModuleRules
{
	public LiveConfig(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange( new string[] { "Core", "HTTP", "Json", "JsonUtilities" } );
			
		
		PrivateDependencyModuleNames.AddRange( new string[] { "CoreUObject", "Engine", "Slate", "SlateCore", "DeveloperSettings" } );

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("SourceControl");
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		RuntimeDependencies.Add("$(ProjectDir)/Config/LiveConfig/...", StagedFileType.NonUFS);
	}
}
