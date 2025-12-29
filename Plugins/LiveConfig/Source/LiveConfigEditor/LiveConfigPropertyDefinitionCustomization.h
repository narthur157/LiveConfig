// All rights reserved - Nicholas Arthur

#pragma once

#include "CoreMinimal.h"
#include "DetailWidgetRow.h"
#include "EdGraphUtilities.h"
#include "UObject/Object.h"
#include "IPropertyTypeCustomization.h"

class LIVECONFIGEDITOR_API FLiveConfigPropertyDefinitionCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
 
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
};

class FLiveConfigPropertyPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* Pin) const override;
};
