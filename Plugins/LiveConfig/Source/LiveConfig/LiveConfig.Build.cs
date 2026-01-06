using UnrealBuildTool;

public class LiveConfig : ModuleRules
{
	public LiveConfig(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange( new string[] { "Core", "HTTP" } );
			
		
		PrivateDependencyModuleNames.AddRange( new string[] { "CoreUObject", "Engine", "Slate", "SlateCore", "DeveloperSettings" } );
	}
}
