#include "LiveConfigRowNameCustomization.h"

#include "DetailWidgetRow.h"
#include "LiveConfigSystem.h"
#include "PropertyCustomizationHelpers.h"
#include "LiveConfigRowPicker.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UnrealType.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<IPropertyTypeCustomization> FLiveConfigRowNameCustomization::MakeInstance()
{
    return MakeShareable(new FLiveConfigRowNameCustomization);
}

void FLiveConfigRowNameCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
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
                FLiveConfigRowName CurrentRowName = GetCurrentRowName();
                TSharedRef<SLiveConfigRowPicker> Widget = SNew(SLiveConfigRowPicker)
                    .bReadOnly(false)
                    .OnRowNameChanged_Lambda([this](FLiveConfigRowName NewRowName)
                    {
                        OnRowNameChanged(NewRowName);
                        if (ComboButton.IsValid())
                        {
                            ComboButton->SetIsOpen(false);
                        }
                    });
                if (CurrentRowName.IsValid())
                {
                    Widget->SetSelectedRowName(CurrentRowName);
                }
                return Widget;
            })
            .ContentPadding(FMargin(2.0f, 2.0f))
            .ButtonContent()
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    FName CurrentRowName = GetCurrentRowName();
                    if (CurrentRowName != NAME_None)
                    {
                        return FText::FromName(CurrentRowName);
                    }
                    return NSLOCTEXT("LiveConfig", "None", "None");
                })
                .ToolTipText_Lambda([this]()
                {
                    FName CurrentRowName = GetCurrentRowName();
                    if (CurrentRowName != NAME_None)
                    {
                        return FText::FromName(CurrentRowName);
                    }
                    return FText::GetEmpty();
                })
            ]
        ];
}

void FLiveConfigRowNameCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    // No children to customize
}

void FLiveConfigRowNameCustomization::OnRowNameChanged(FLiveConfigRowName NewRowName)
{
    SetRowName(NewRowName);
}

FName FLiveConfigRowNameCustomization::GetCurrentRowName() const
{
    if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
    {
        return NAME_None;
    }

    TArray<void*> RawData;
    PropertyHandle->AccessRawData(RawData);

    if (RawData.Num() > 0)
    {
        // FLiveConfigRowName is a struct, so we need to access the RowName member
        // We'll need to get the struct property and then the RowName property
        if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
        {
            if (FProperty* RowNameProperty = StructProperty->Struct->FindPropertyByName(TEXT("RowName")))
            {
                if (FNameProperty* NameProp = CastField<FNameProperty>(RowNameProperty))
                {
                    FName* RowNamePtr = NameProp->GetPropertyValuePtr_InContainer(RawData[0]);
                    if (RowNamePtr)
                    {
                        return *RowNamePtr;
                    }
                }
            }
        }
    }

    return NAME_None;
}

void FLiveConfigRowNameCustomization::SetRowName(FLiveConfigRowName NewRowName)
{
    if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
    {
        return;
    }

    // Set the RowName member of the FLiveConfigRowName struct
    TArray<void*> RawData;
    PropertyHandle->AccessRawData(RawData);

    if (RawData.Num() > 0)
    {
        if (FStructProperty* StructProperty = CastField<FStructProperty>(PropertyHandle->GetProperty()))
        {
            if (FProperty* RowNameProperty = StructProperty->Struct->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FLiveConfigRowName, RowName)))
            {
                if (FNameProperty* NameProp = CastField<FNameProperty>(RowNameProperty))
                {
                    FName* RowNamePtr = NameProp->GetPropertyValuePtr_InContainer(RawData[0]);
                    if (RowNamePtr)
                    {
                        *RowNamePtr = NewRowName.GetRowName();
                        PropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                }
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION