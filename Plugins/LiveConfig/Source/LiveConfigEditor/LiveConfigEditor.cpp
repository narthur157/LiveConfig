#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyCustomization.h"
#include "LiveConfigPropertyPinFactory.h"
#include "LiveConfigSystem.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

void FLiveConfigEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register customization for FLiveConfigProperty
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLiveConfigProperty::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLiveConfigPropertyCustomization::MakeInstance)
	);

	auto PropertyPinFactory = MakeShareable(new FLiveConfigPropertyPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(PropertyPinFactory);
	FLiveConfigPropertyStyle::Initialize();
}

void FLiveConfigEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigProperty::StaticStruct()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor)