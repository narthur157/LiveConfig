// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigPropertyCombo.h"

#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyChip.h"
#include "LiveConfigPropertyPicker.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyCombo)

void SLiveConfigPropertyCombo::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
    SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Property", RowNameAttribute, EInvalidateWidgetReason::Layout);
}

SLiveConfigPropertyCombo::SLiveConfigPropertyCombo() : RowNameAttribute(*this)
{
}

void SLiveConfigPropertyCombo::Construct(const FArguments& InArgs)
{
    RowNameAttribute.Assign(*this, InArgs._Property);
	OnPropertyChanged = InArgs._OnPropertyChanged;

    ChildSlot
    [
        SAssignNew(ComboButton, SComboButton)
        .ComboButtonStyle(FLiveConfigPropertyStyle::Get(), "LiveConfigProperty.ComboButton")
        .HasDownArrow(true)
        .ContentPadding(1)
        .Clipping(EWidgetClipping::OnDemand) 
        .OnGetMenuContent_Lambda([this]()
        {
            return SNew(SLiveConfigPropertyPicker)
                .bReadOnly(false)
                .OnPropertyChanged(this, &SLiveConfigPropertyCombo::OnPropertySelected);
        })
        .ButtonContent()
        [
            SAssignNew(Chip, SLiveConfigPropertyChip)
            .ShowClearButton(this, &SLiveConfigPropertyCombo::ShowClearButton)
            .OnEditPressed_Lambda([&]
            {
                if (ComboButton->IsOpen())
                {
                    ComboButton->SetIsOpen(false); 
                }
                else
                {
                    ComboButton->SetIsOpen(ComboButton->ShouldOpenDueToClick()); 
                }
                return FReply::Handled();
            })
            .OnClearPressed_Lambda([&]
            {
                ComboButton->SetIsOpen(false);
                OnPropertyChanged.ExecuteIfBound({});
                return FReply::Handled();
            })
            .ReadOnly(false)
            .Text_Lambda([this]()
            {
                FLiveConfigProperty CurrentProperty = RowNameAttribute.Get();
                if (CurrentProperty.IsValid())
                {
                    return FText::FromName(CurrentProperty.GetName());
                }
                return NSLOCTEXT("LiveConfig", "None", "None");
            })
        ]
    ];
}

bool SLiveConfigPropertyCombo::ShowClearButton() const
{
    return RowNameAttribute.Get().IsValid();
}

void SLiveConfigPropertyCombo::OnPropertySelected(FLiveConfigProperty Property)
{
    if (Property.IsValid())
    {
        OnPropertyChanged.ExecuteIfBound(Property);
        ComboButton->SetIsOpen(false);
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
