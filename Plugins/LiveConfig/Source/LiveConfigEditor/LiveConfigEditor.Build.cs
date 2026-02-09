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
                "SourceControl"
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
                "Kismet",
                "KismetCompiler",
                "EditorStyle",
                "InputCore",
                "PythonScriptPlugin",
                "ToolMenus",
                "ApplicationCore",
                "StructViewer",
                "DesktopPlatform",
                "AssetRegistry",
                "EditorSubsystem"
            }
        );
    }
}
