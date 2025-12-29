#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "LiveConfigPropertyName.h"

class FLiveConfigRowNameCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    /** IPropertyTypeCustomization interface */
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    /** Handle when the row name changes in the widget */
    void OnRowNameChanged(FLiveConfigRowName NewRowName);

    /** Get the current row name from the property */
    FName GetCurrentRowName() const;

    /** Set the row name in the property */
    void SetRowName(FLiveConfigRowName NewRowName);

    TSharedPtr<IPropertyHandle> PropertyHandle;
    TSharedPtr<class SComboButton> ComboButton;
};

