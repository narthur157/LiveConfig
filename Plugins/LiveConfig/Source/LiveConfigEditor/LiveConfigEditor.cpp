#include "LiveConfigEditor.h"

#include "EdGraphUtilities.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigPropertyStyle.h"
#include "LiveConfigRowNameCustomization.h"
#include "LiveConfigRowNamePinFactory.h"
#include "LiveConfigSystem.h"

#define LOCTEXT_NAMESPACE "FLiveConfigEditorModule"

void FLiveConfigEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register customization for FLiveConfigRowName
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FLiveConfigRowName::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FLiveConfigRowNameCustomization::MakeInstance)
	);

	auto RowNamePinFactory = MakeShareable(new FLiveConfigRowNamePinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(RowNamePinFactory);
	FLiveConfigPropertyStyle::Initialize();
}

void FLiveConfigEditorModule::ShutdownModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigPropertyDefinition::StaticStruct()->GetFName());
	PropertyModule.UnregisterCustomPropertyTypeLayout(FLiveConfigRowName::StaticStruct()->GetFName());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FLiveConfigEditorModule, LiveConfigEditor)