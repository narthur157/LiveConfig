// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigPropertyChip.h"

#include "LiveConfigPropertyStyle.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "LiveConfigPropertyChip"
SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyChip);

void SLiveConfigPropertyChip::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "TooltipText", ToolTipTextAttribute, EInvalidateWidgetReason::Layout);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "Text", TextAttribute, EInvalidateWidgetReason::Layout);
	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "ShowClearButton", ShowClearButtonAttribute, EInvalidateWidgetReason::Layout);

	SLATE_ADD_MEMBER_ATTRIBUTE_DEFINITION_WITH_NAME(AttributeInitializer, "IsSelected", IsSelectedAttribute, EInvalidateWidgetReason::Layout)
	.OnValueChanged(FSlateAttributeDescriptor::FAttributeValueChangedDelegate::CreateLambda([](SWidget& Widget)
		{
			static_cast<SLiveConfigPropertyChip&>(Widget).UpdatePillStyle();
		}));
}

SLiveConfigPropertyChip::SLiveConfigPropertyChip()
	: ToolTipTextAttribute(*this)
	, TextAttribute(*this)
	, IsSelectedAttribute(*this)
	, ShowClearButtonAttribute(*this)
{
}

void SLiveConfigPropertyChip::Construct(const FArguments& InArgs)
{
	TextAttribute.Assign(*this, InArgs._Text);
	ToolTipTextAttribute.Assign(*this, InArgs._ToolTipText);
	ShowClearButtonAttribute.Assign(*this, InArgs._ShowClearButton);
	IsSelectedAttribute.Assign(*this, InArgs._IsSelected);
	
	OnEditPressed = InArgs._OnEditPressed;
	OnClearPressed = InArgs._OnClearPressed;
	
	TWeakPtr<SLiveConfigPropertyChip> WeakSelf = StaticCastWeakPtr<SLiveConfigPropertyChip>(AsWeak());

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(18.0f)
		[
			SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(FStyleDefaults::GetNoBrush())
			[
				SNew(SHorizontalBox)
				
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Fill)
				.AutoWidth()
				[
					SAssignNew(ChipButton, SButton)
					.ContentPadding(0)
					.ToolTipText_Lambda([WeakSelf]()
					{
						const TSharedPtr<SLiveConfigPropertyChip> Self = WeakSelf.Pin();
						return Self.IsValid() ? Self->ToolTipTextAttribute.Get() : FText::GetEmpty();
					})
					.IsEnabled(!InArgs._ReadOnly)
					.OnClicked_Lambda([WeakSelf]()
					{
						if (const TSharedPtr<SLiveConfigPropertyChip> Self = WeakSelf.Pin())
						{
							const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();

							if (Self->OnEditPressed.IsBound())
							{
								return Self->OnEditPressed.Execute();
							}
						}
						
						return FReply::Unhandled();
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(FAppStyle::GetFontStyle( TEXT("PropertyWindow.NormalFont")))
							.Text_Lambda([WeakSelf]()
							{
								const TSharedPtr<SLiveConfigPropertyChip> Self = WeakSelf.Pin();
								return Self.IsValid() ? Self->TextAttribute.Get() : FText::GetEmpty();
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0)
						[
							SAssignNew(ClearButton, SButton)
							.IsEnabled(!InArgs._ReadOnly)
							.Visibility_Lambda([&]
							{
								return ShowClearButtonAttribute.Get() ? EVisibility::Visible : EVisibility::Collapsed;
							})	
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.ButtonStyle(FLiveConfigPropertyStyle::Get(), "LiveConfigProperty.ChipClearButton")
							.ToolTipText(LOCTEXT("ClearTag", "Clear Property"))
							.ContentPadding(FMargin(2, 0, 0, 0))
							.OnClicked_Lambda([WeakSelf]()
							{
								const TSharedPtr<SLiveConfigPropertyChip> Self = WeakSelf.Pin();
								if (Self.IsValid() && Self->OnClearPressed.IsBound())
								{
									return Self->OnClearPressed.Execute();
								}
								return FReply::Unhandled();
							})
							[
								SNew(SImage)
								.ColorAndOpacity_Lambda([WeakSelf]()
								{
									const TSharedPtr<SLiveConfigPropertyChip> Self = WeakSelf.Pin();
									if (Self.IsValid() && Self->ClearButton.IsValid())
									{
										return Self->ClearButton->IsHovered() ? FStyleColors::White : FStyleColors::Foreground;
									}
									return FStyleColors::Foreground;
								})
								.Image(FAppStyle::GetBrush("Icons.X"))
								.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
							]
						]
					]
				]
			]
		]
	];
}

void SLiveConfigPropertyChip::UpdatePillStyle()
{
	const bool bIsSelected = IsSelectedAttribute.Get();
	if (bIsSelected != bLastHasIsSelected)
	{
		if (bIsSelected)
		{
			ChipButton->SetButtonStyle(&FLiveConfigPropertyStyle::Get().GetWidgetStyle<FButtonStyle>("LiveConfigProperty.ChipButton.Selected"));
		}
		else
		{
			ChipButton->SetButtonStyle(&FLiveConfigPropertyStyle::Get().GetWidgetStyle<FButtonStyle>("LiveConfigProperty.ChipButton.Unselected"));
		}
		bLastHasIsSelected = bIsSelected;
	}	
}

#undef LOCTEXT_NAMESPACE