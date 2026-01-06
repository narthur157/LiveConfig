#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyCustomization.h"
#include "LiveConfigPropertyPinFactory.h"
#include "LiveConfigSystem.h"
#include "LiveConfigBlueprintExtensions.h"
#include "GraphEditorModule.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

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
}

void FLiveConfigEditorModule::ShutdownModule()
{
	// Shutdown blueprint extensions
	FLiveConfigBlueprintExtensions::Shutdown();

	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigProperty::StaticStruct()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor);