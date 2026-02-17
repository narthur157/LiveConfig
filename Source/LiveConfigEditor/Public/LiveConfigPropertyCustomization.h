// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "LiveConfigPropertyName.h"

class FLiveConfigPropertyCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /** IPropertyTypeCustomization interface */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    /** Handle when the property changes in the widget */
    void OnPropertyChanged(FLiveConfigProperty NewProperty);

    /** Get the current name from the property */
    FName GetCurrentProperty() const;

    /** Set the property in the property */
    void SetProperty(FLiveConfigProperty NewProperty);

    TSharedPtr<IPropertyHandle> PropertyHandle;
    TSharedPtr<class SComboButton> ComboButton;
};

