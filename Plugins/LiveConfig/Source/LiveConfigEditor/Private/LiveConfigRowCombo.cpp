// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigRowCombo.h"

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
        SNew(SComboButton)
            .OnGetMenuContent_Lambda([this]()
            {
                return SNew(SLiveConfigRowPicker)
                    .bReadOnly(false)
                    .OnRowNameChanged(this, &SLiveConfigRowCombo::OnRowNameSelected);
            })
            .ContentPadding(FMargin(2.0f, 2.0f))
            .ButtonContent()
            [
                SNew(STextBlock)
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

void SLiveConfigRowCombo::OnRowNameSelected(FLiveConfigRowName RowName)
{
    if (RowName.IsValid())
    {
        OnRowNameChanged.ExecuteIfBound(RowName);
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
