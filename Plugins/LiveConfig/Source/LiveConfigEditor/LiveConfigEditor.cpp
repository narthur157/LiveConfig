#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyDefinitionCustomization.h"
#include "LiveConfigSystem.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

void FLiveConfigEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLiveConfigPropertyDefinition::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLiveConfigPropertyDefinitionCustomization::MakeInstance)
	);

	auto PropertyPinFactory = MakeShareable(new FLiveConfigPropertyPinFactory());

	// Register it
	FEdGraphUtilities::RegisterVisualPinFactory(PropertyPinFactory);
}

void FLiveConfigEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());   
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor)