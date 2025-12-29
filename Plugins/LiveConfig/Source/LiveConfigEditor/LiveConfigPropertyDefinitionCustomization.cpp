// All rights reserved - Nicholas Arthur


#include "LiveConfigPropertyDefinitionCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "LiveConfigSystem.h"
#include "EdGraphUtilities.h"
#include "SGraphPin.h"
#include "SLiveConfigPropertyDefinitionPin.h"

TSharedRef<IPropertyTypeCustomization> FLiveConfigPropertyDefinitionCustomization::MakeInstance()
{
	return MakeShared<FLiveConfigPropertyDefinitionCustomization>();
}
 
void FLiveConfigPropertyDefinitionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			//Create Slate widgets to display in the Name column of the
			//property editor here - a good default is to call the
			//CreatePropertyNameWidget like below, as it generates
			//the standard name column for the property.
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			//Create Slate widgets to display the value of the property here.
			//What you need to put here depends entirely on what you are doing.
			SNew(STextBlock).Text(INVTEXT("Hello"))
		];
}
 
void FLiveConfigPropertyDefinitionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

TSharedPtr<class SGraphPin> FLiveConfigPropertyPinFactory::CreatePin(class UEdGraphPin* Pin) const
{
	if (Pin == nullptr)
	{
		return nullptr;
	}

	// Check if the pin is the struct type you want to customize
	// This is the most reliable way to check for a specific UStruct
	if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct &&
		Pin->PinType.PinSubCategoryObject == FLiveConfigPropertyDefinition::StaticStruct())
	{
		// If it is, return an instance of your custom SGraphPin
		return SNew(SLiveConfigPropertyDefinitionPin, Pin);
	}

	// If it's not your type, return nullptr so the engine can
	// use its default factory.
	return nullptr;
}
