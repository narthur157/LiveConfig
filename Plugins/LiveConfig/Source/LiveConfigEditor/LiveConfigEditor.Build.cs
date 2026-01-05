using UnrealBuildTool;

public class LiveConfigEditor : ModuleRules
{
    public LiveConfigEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "LiveConfig", 
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "GraphEditor",
                "UnrealEd",
                "BlueprintGraph", 
                "EditorStyle",
                "InputCore"
            }
        );
    }
}