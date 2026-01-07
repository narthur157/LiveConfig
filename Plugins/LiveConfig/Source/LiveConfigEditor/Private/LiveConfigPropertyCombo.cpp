// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigPropertyCombo.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "HAL/PlatformApplicationMisc.h"
#include "LiveConfigSystem.h"

#include "LiveConfigPropertyStyle.h"
#include "LiveConfigPropertyChip.h"
#include "LiveConfigPropertyPicker.h"
#include "LiveConfigGameSettings.h"
#include "SlateOptMacros.h"
#include "LiveConfigEditor/LiveConfigEditor.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyCombo)

void SLiveConfigPropertyCombo::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
    SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Property", RowNameAttribute, EInvalidateWidgetReason::Layout);
    SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "FilterType", FilterTypeAttribute, EInvalidateWidgetReason::Layout);
}

SLiveConfigPropertyCombo::SLiveConfigPropertyCombo() : RowNameAttribute(*this), FilterTypeAttribute(*this)
{
}

void SLiveConfigPropertyCombo::Construct(const FArguments& InArgs)
{
    RowNameAttribute.Assign(*this, InArgs._Property);
    FilterTypeAttribute.Assign(*this, InArgs._FilterType);
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
                .FilterType(FilterTypeAttribute.Get())
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
                    FString DisplayString = CurrentProperty.GetName().ToString();
                    if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
                    {
                        if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(CurrentProperty))
                        {
                            DisplayString += FString::Printf(TEXT(": %s"), *Def->Value);
                        }
                    }
                    return FText::FromString(DisplayString);
                }
                return NSLOCTEXT("LiveConfig", "None", "None");
            })
        ]
    ];
}

FReply SLiveConfigPropertyCombo::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return FReply::Handled();
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SLiveConfigPropertyCombo::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		TSharedPtr<SWidget> MenuContent = OnGetContextMenuContent();
		if (MenuContent.IsValid())
		{
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContent.ToSharedRef(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
			return FReply::Handled();
		}
	}

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

bool SLiveConfigPropertyCombo::ShowClearButton() const
{
    return RowNameAttribute.Get().IsValid();
}

void SLiveConfigPropertyCombo::OnPropertySelected(FLiveConfigProperty Property)
{
    OnPropertyChanged.ExecuteIfBound(Property);
    ComboButton->SetIsOpen(false);
}

TSharedPtr<SWidget> SLiveConfigPropertyCombo::OnGetContextMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("LiveConfig", "CopyProperty", "Copy"),
		NSLOCTEXT("LiveConfig", "CopyPropertyTooltip", "Copy the property name to the clipboard"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Copy"),
		FUIAction(FExecuteAction::CreateSP(this, &SLiveConfigPropertyCombo::OnCopyProperty))
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("LiveConfig", "PasteProperty", "Paste"),
		NSLOCTEXT("LiveConfig", "PastePropertyTooltip", "Paste the property name from the clipboard"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Paste"),
		FUIAction(
			FExecuteAction::CreateSP(this, &SLiveConfigPropertyCombo::OnPasteProperty),
			FCanExecuteAction::CreateSP(this, &SLiveConfigPropertyCombo::CanPasteProperty)
		)
	);

	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("LiveConfig", "ClearProperty", "Clear"),
		NSLOCTEXT("LiveConfig", "ClearPropertyTooltip", "Clear the current property selection"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Delete"),
		FUIAction(FExecuteAction::CreateSP(this, &SLiveConfigPropertyCombo::OnClearProperty))
	);

	return MenuBuilder.MakeWidget();
}

void SLiveConfigPropertyCombo::OnCopyProperty()
{
	FLiveConfigProperty CurrentProperty = RowNameAttribute.Get();
	if (CurrentProperty.IsValid())
	{
		FPlatformApplicationMisc::ClipboardCopy(*CurrentProperty.ToString());
	}
}

void SLiveConfigPropertyCombo::OnPasteProperty()
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

	if (!ClipboardText.IsEmpty())
	{
		FLiveConfigProperty NewProperty(ClipboardText);
		if (ULiveConfigSystem::Get()->DoesPropertyNameExist(NewProperty))
		{
			OnPropertyChanged.ExecuteIfBound(NewProperty);
		}
	}
}

bool SLiveConfigPropertyCombo::CanPasteProperty() const
{
	FString ClipboardText;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardText);

	if (!ClipboardText.IsEmpty())
	{
		FLiveConfigProperty NewProperty(ClipboardText);
		return ULiveConfigSystem::Get()->DoesPropertyNameExist(NewProperty);
	}

	return false;
}

void SLiveConfigPropertyCombo::OnClearProperty()
{
	OnPropertyChanged.ExecuteIfBound(FLiveConfigProperty());
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
