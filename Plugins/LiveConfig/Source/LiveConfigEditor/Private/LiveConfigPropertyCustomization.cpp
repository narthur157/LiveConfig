#include "LiveConfigPropertyCustomization.h"

#include "DetailWidgetRow.h"
#include "LiveConfigSystem.h"
#include "PropertyCustomizationHelpers.h"
#include "LiveConfigPropertyPicker.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UnrealType.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<IPropertyTypeCustomization> FLiveConfigPropertyCustomization::MakeInstance()
{
    return MakeShareable(new FLiveConfigPropertyCustomization);
}

void FLiveConfigPropertyCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    PropertyHandle = InPropertyHandle;

    HeaderRow
        .NameContent()
        [
            InPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(250.0f)
        [
            SAssignNew(ComboButton, SComboButton)
            .OnGetMenuContent_Lambda([this]()
            {
                FLiveConfigProperty CurrentProperty = GetCurrentProperty();
                TSharedRef<SLiveConfigPropertyPicker> Widget = SNew(SLiveConfigPropertyPicker)
                    .bReadOnly(false)
                    .OnPropertyChanged_Lambda([this](FLiveConfigProperty NewProperty)
                    {
                        OnPropertyChanged(NewProperty);
                        if (ComboButton.IsValid())
                        {
                            ComboButton->SetIsOpen(false);
                        }
                    });
                if (CurrentProperty.IsValid())
                {
                    Widget->SetSelectedProperty(CurrentProperty);
                }
                return Widget;
            })
            .ContentPadding(FMargin(2.0f, 2.0f))
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    FName CurrentPropertyName = GetCurrentProperty();
                    if (CurrentPropertyName != NAME_None)
                    {
                        return FText::FromName(CurrentPropertyName);
                    }
                    return NSLOCTEXT("LiveConfig", "None", "None");
                })
                .ToolTipText_Lambda([this]()
                {
                    FName CurrentPropertyName = GetCurrentProperty();
                    if (CurrentPropertyName != NAME_None)
                    {
                        return FText::FromName(CurrentPropertyName);
                    }
                    return FText::GetEmpty();
                })
            ]
        ];
}

void FLiveConfigPropertyCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // No children to customize
}

void FLiveConfigPropertyCustomization::OnPropertyChanged(FLiveConfigProperty NewProperty)
{
    SetProperty(NewProperty);
}

FName FLiveConfigPropertyCustomization::GetCurrentProperty() const
{
    if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
    {
        return NAME_None;
    }

    TArray<void*> RawData;
    PropertyHandle->AccessRawData(RawData);

    if (RawData.Num() > 0)
    {
        // FLiveConfigProperty is a struct, so we need to access the RowName member
        // We'll need to get the struct property and then the RowName property
        if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
        {
            if (FProperty* PropertyProperty = StructProperty->Struct->FindPropertyByName(TEXT("PropertyName")))
            {
                if (FNameProperty* NameProp = CastField<FNameProperty>(PropertyProperty))
                {
                    FName* NamePtr = NameProp->GetPropertyValuePtr_InContainer(RawData[0]);
                    if (NamePtr)
                    {
                        return *NamePtr;
                    }
                }
            }
        }
    }

    return NAME_None;
}

void FLiveConfigPropertyCustomization::SetProperty(FLiveConfigProperty NewProperty)
{
    if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
    {
        return;
    }

    // Set the RowName member of the FLiveConfigProperty struct
    TArray<void*> RawData;
    PropertyHandle->AccessRawData(RawData);

    if (RawData.Num() > 0)
    {
        if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
        {
            if (FProperty* PropertyProperty = StructProperty->Struct->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FLiveConfigProperty, PropertyName)))
            {
                if (FNameProperty* NameProp = CastField<FNameProperty>(PropertyProperty))
                {
                    FName* NamePtr = NameProp->GetPropertyValuePtr_InContainer(RawData[0]);
                    if (NamePtr)
                    {
                        *NamePtr = NewProperty.GetName();
                        PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                }
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION