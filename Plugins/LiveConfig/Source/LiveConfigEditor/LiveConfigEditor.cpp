#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyCustomization.h"
#include "LiveConfigPropertyPinFactory.h"
#include "LiveConfigSystem.h"
#include "LiveConfigBlueprintExtensions.h"
#include "GraphEditorModule.h"
#include "SLiveConfigPropertyManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

static const FName LiveConfigPropertyManagerTabId("LiveConfigPropertyManager");

void FLiveConfigEditorModule::StartupModule()
{
	UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigEditorModule::StartupModule()"));

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register customization for FLiveConfigProperty
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLiveConfigProperty::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLiveConfigPropertyCustomization::MakeInstance)
	);

	auto PropertyPinFactory = MakeShareable(new FLiveConfigPropertyPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(PropertyPinFactory);
	FLiveConfigPropertyStyle::Initialize();

	// Initialize blueprint extensions for "Promote to Live Config"
	FLiveConfigBlueprintExtensions::Initialize();

	// Register FExtender fallback
	FGraphEditorModule& GraphEditorModule = FModuleManager::LoadModuleChecked<FGraphEditorModule>("GraphEditor");
	TArray<FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode>& MenuExtenderList = GraphEditorModule.GetAllGraphEditorContextMenuExtender();
	MenuExtenderList.Add(FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode::CreateStatic(&FLiveConfigBlueprintExtensions::OnExtendPinMenu));

	RegisterTabSpawners();

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLiveConfigEditorModule::RegisterMenus));
}

void FLiveConfigEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UnregisterTabSpawners();

	// Shutdown blueprint extensions
	FLiveConfigBlueprintExtensions::Shutdown();

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigProperty::StaticStruct()->GetFName());
}

void FLiveConfigEditorModule::OpenPropertyManager(FLiveConfigProperty FocusProperty)
{
	PropertyToFocus = FocusProperty;
	TSharedPtr<SDockTab> Tab = FGlobalTabmanager::Get()->TryInvokeTab(LiveConfigPropertyManagerTabId);
	if (Tab.IsValid() && PropertyToFocus.IsValid())
	{
		TSharedRef<SLiveConfigPropertyManager> Manager = StaticCastSharedRef<SLiveConfigPropertyManager>(Tab->GetContent());
		Manager->ScrollToProperty(PropertyToFocus);
		PropertyToFocus = FLiveConfigProperty();
	}
}

void FLiveConfigEditorModule::RegisterTabSpawners()
{
	FTabSpawnerEntry& Spawner = FGlobalTabmanager::Get()->RegisterTabSpawner(LiveConfigPropertyManagerTabId, FOnSpawnTab::CreateRaw(this, &FLiveConfigEditorModule::SpawnPropertyManagerTab))
		.SetDisplayName(LOCTEXT("PropertyManagerTabTitle", "Live Config Property Manager"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FLiveConfigEditorModule::UnregisterTabSpawners()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(LiveConfigPropertyManagerTabId);
}

TSharedRef<SDockTab> FLiveConfigEditorModule::SpawnPropertyManagerTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SLiveConfigPropertyManager> Manager = SNew(SLiveConfigPropertyManager);
	if (PropertyToFocus.IsValid())
	{
		Manager->ScrollToProperty(PropertyToFocus);
		PropertyToFocus = FLiveConfigProperty();
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		[
			Manager
		];
}

void FLiveConfigEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* PlayMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (PlayMenu)
	{
		FToolMenuSection& Section = PlayMenu->AddSection("LiveConfig", LOCTEXT("LiveConfigSection", "Live Config"));
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"OpenLiveConfigManager",
			FUIAction(FExecuteAction::CreateRaw(this, &FLiveConfigEditorModule::OpenPropertyManager, FLiveConfigProperty())),
			LOCTEXT("OpenLiveConfigManager", "Live Config"),
			LOCTEXT("OpenLiveConfigManagerTooltip", "Open the Live Config Property Manager"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
		));
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor);