// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "SLiveConfigTagRow.h"

#include "LiveConfigLib.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigTagRow);

void SLiveConfigTagRow::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigTagRow::Construct(const FArguments& InArgs, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex, FSimpleDelegate InOnRemove, bool bInReadOnly)
{
	Item = InItem;
	Index = InIndex;
	OnRemove = InOnRemove;
	bReadOnly = bInReadOnly;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(4, 2))
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor_Lambda([this]() 
				{ 
					if (Item.IsValid() && Item->Tags.IsValidIndex(Index))
					{
						return ULiveConfigLib::GetTagColor(Item->Tags[Index]);
					}
					return FSlateColor(FLinearColor::White);
				})
				[
					SNew(SBox)
					.WidthOverride(8.0f)
					.HeightOverride(8.0f)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					if (Item.IsValid() && Item->Tags.IsValidIndex(Index))
					{
						return FText::FromName(Item->Tags[Index]);
					}
					return FText::GetEmpty();
				})
				.Font(FAppStyle::GetFontStyle("NormalFont"))
				.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.Visibility_Lambda([this]() { return bReadOnly ? EVisibility::Collapsed : EVisibility::Visible; })
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(0)
				.OnClicked_Lambda([this]()
				{
					Item->Tags.RemoveAt(Index);
					OnRemove.ExecuteIfBound();
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.X"))
					.DesiredSizeOverride(FVector2D(10, 10))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
