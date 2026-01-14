#include "SLiveConfigTagRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigTagRow);

void SLiveConfigTagRow::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigTagRow::Construct(const FArguments& InArgs, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex, FSimpleDelegate InOnChanged, TFunction<FSlateColor(FName)> InGetTagColor)
{
	Item = InItem;
	Index = InIndex;
	OnChanged = InOnChanged;

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
				.BorderBackgroundColor_Lambda([InGetTagColor, this]() 
				{ 
					if (Item.IsValid() && Item->Tags.IsValidIndex(Index))
					{
						return InGetTagColor(Item->Tags[Index]);
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
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(0)
				.OnClicked_Lambda([this]()
				{
					Item->Tags.RemoveAt(Index);
					OnChanged.ExecuteIfBound();
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
