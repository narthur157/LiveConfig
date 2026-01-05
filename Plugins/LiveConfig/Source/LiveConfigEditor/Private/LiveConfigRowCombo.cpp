// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigRowCombo.h"

#include "LiveConfigPropertyStyle.h"
#include "LiveConfigRowChip.h"
#include "LiveConfigRowPicker.h"
#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SLATE_IMPLEMENT_WIDGET(SLiveConfigRowCombo)

void SLiveConfigRowCombo::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
    SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "RowName", RowNameAttribute, EInvalidateWidgetReason::Layout);
}

SLiveConfigRowCombo::SLiveConfigRowCombo() : RowNameAttribute(*this)
{
}

void SLiveConfigRowCombo::Construct(const FArguments& InArgs)
{
    RowNameAttribute.Assign(*this, InArgs._RowName);
	OnRowNameChanged = InArgs._OnRowNameChanged;

    ChildSlot
    [
        SAssignNew(ComboButton, SComboButton)
        .ComboButtonStyle(FLiveConfigPropertyStyle::Get(), "LiveConfigProperty.ComboButton")
        .HasDownArrow(true)
        .ContentPadding(1)
        .Clipping(EWidgetClipping::OnDemand) 
        .OnGetMenuContent_Lambda([this]()
        {
            return SNew(SLiveConfigRowPicker)
                .bReadOnly(false)
                .OnRowNameChanged(this, &SLiveConfigRowCombo::OnRowNameSelected);
        })
        .ButtonContent()
        [
            SAssignNew(Chip, SLiveConfigRowChip)
            .ShowClearButton(this, &SLiveConfigRowCombo::ShowClearButton)
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
                OnRowNameChanged.ExecuteIfBound({});
                return FReply::Handled();
            })
            .ReadOnly(false)
            .Text_Lambda([this]()
            {
                FLiveConfigRowName CurrentRowName = RowNameAttribute.Get();
                if (CurrentRowName.IsValid())
                {
                    return FText::FromName(CurrentRowName.GetRowName());
                }
                return NSLOCTEXT("LiveConfig", "None", "None");
            })
        ]
    ];
}

bool SLiveConfigRowCombo::ShowClearButton() const
{
    return RowNameAttribute.Get().IsValid();
}

void SLiveConfigRowCombo::OnRowNameSelected(FLiveConfigRowName RowName)
{
    if (RowName.IsValid())
    {
        OnRowNameChanged.ExecuteIfBound(RowName);
        ComboButton->SetIsOpen(false);
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
