#include "SLiveConfigPropertyRow.h"
#include "SLiveConfigTagRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/SlateStyleMacros.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyRow);

void SLiveConfigPropertyRow::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigPropertyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex)
{
	Item = InItem;
	OnDeleteProperty = InArgs._OnDeleteProperty;
	OnIsNameDuplicate = InArgs._IsNameDuplicate;
	OnChanged = InArgs._OnChanged;
	GetTagColor = InArgs._GetTagColor;
	KnownTagsAttribute.Assign(*this, InArgs._KnownTags);
	
	SMultiColumnTableRow<TSharedPtr<FLiveConfigPropertyDefinition>>::Construct(
		SPropertyRowParent::FArguments()
		.Padding(FMargin(0, 4)), 
		InOwnerTable);

	if (InIndex % 2 == 1)
	{
		// Use a subtle dark background for striping
		SetBorderImage(FAppStyle::GetBrush("WhiteBrush"));
		SetBorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.02f));
	}
	else
	{
		SetBorderImage(FAppStyle::GetBrush("NoBorder"));
	}

	// If the name is empty, it's a new property, request focus
	if (!Item->PropertyName.IsValid())
	{
		bNeedsFocus = true;
	}
}

void SLiveConfigPropertyRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPropertyRowParent::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bNeedsFocus)
	{
		if (NameTextBox.IsValid())
		{
			TSharedRef<SEditableTextBox> NameTextBoxRef = NameTextBox.ToSharedRef();
			FSlateApplication::Get().SetKeyboardFocus(NameTextBoxRef, EFocusCause::SetDirectly);
			bNeedsFocus = false;
		}
	}
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == "Actions")
	{
		return SNew(SBox)
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					OnDeleteProperty.ExecuteIfBound(Item);
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("DeleteProperty", "Delete Property"))
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Delete"))
				]
			];
	}
	else if (ColumnName == "Name")
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 2.0f))
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					.ToolTipText_Lambda([this]() { return FText::FromString(Item->Description); })
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(NameTextBox, SEditableTextBox)
						.Font(FAppStyle::GetFontStyle("BoldFont"))
						.ForegroundColor(FLinearColor::Black)
						.Text_Lambda([this]()
						{
							return Item->PropertyName.IsValid() ? FText::FromName(Item->PropertyName.GetName()) : FText::GetEmpty();
						})
						.OnVerifyTextChanged_Lambda([](const FText& NewText, FText& OutError)
						{
							FString TextStr = NewText.ToString();
							if (TextStr.Contains(TEXT(" ")))
							{
								OutError = LOCTEXT("PropertyNameSpaceError", "Property names cannot contain spaces.");
								return false;
							}
							return true;
						})
						.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
						{
							Item->PropertyName = FLiveConfigProperty(FName(*NewText.ToString()));
							OnChanged.ExecuteIfBound();
						})
						.ForegroundColor_Lambda([this]()
						{
							if (OnIsNameDuplicate.IsBound() && OnIsNameDuplicate.Execute(Item->PropertyName.GetName()))
							{
								return FSlateColor(FLinearColor::Red);
							}
							return FSlateColor::UseForeground();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 0)
					[
						SNew(SCheckBox)
						.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
						.ToolTipText(LOCTEXT("EditDescriptionToolTip", "Edit Description"))
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Info"))
							.DesiredSizeOverride(FVector2D(12, 12))
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
						.IsChecked_Lambda([this]() { return bShowDescription ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bShowDescription = NewState == ECheckBoxState::Checked; })
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return bShowDescription ? EVisibility::Visible : EVisibility::Collapsed; })
					.Padding(FMargin(0, 2, 0, 0))
					[
						SNew(SEditableTextBox)
						.Style(FAppStyle::Get(), "SimpleEditableTextBox")
						.Font(FAppStyle::GetFontStyle("NormalFont"))
						.ForegroundColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f))
						.HintText(LOCTEXT("DescriptionHint", "Add description..."))
						.Text(FText::FromString(Item->Description))
						.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
						{
							Item->Description = NewText.ToString();
							OnChanged.ExecuteIfBound();
						})
					]
				]
			];
	}
	else if (ColumnName == "Description")
	{
		return SNullWidget::NullWidget;
	}
	else if (ColumnName == "Type")
	{
		static TArray<TSharedPtr<ELiveConfigPropertyType>> TypeOptions;
		if (TypeOptions.Num() == 0)
		{
			TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::String));
			TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Int));
			TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Float));
			TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Bool));
		}

		return SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(SComboBox<TSharedPtr<ELiveConfigPropertyType>>)
				.OptionsSource(&TypeOptions)
				.OnGenerateWidget_Lambda([](TSharedPtr<ELiveConfigPropertyType> InType)
				{
					FString TypeStr = StaticEnum<ELiveConfigPropertyType>()->GetNameStringByValue((int64)*InType);
					return SNew(STextBlock).Text(FText::FromString(TypeStr));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<ELiveConfigPropertyType> NewType, ESelectInfo::Type)
				{
					if (NewType.IsValid())
					{
						Item->PropertyType = *NewType;
						switch (Item->PropertyType)
						{
						case ELiveConfigPropertyType::String:
							Item->Value = "";
							break;
						case ELiveConfigPropertyType::Int:
						case ELiveConfigPropertyType::Float:
							Item->Value = "0";
							break;
						case ELiveConfigPropertyType::Bool:
							Item->Value = "false";
							break;
						}
						OnChanged.ExecuteIfBound();
					}
				})
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						FString TypeStr = StaticEnum<ELiveConfigPropertyType>()->GetNameStringByValue((int64)Item->PropertyType);
						return FText::FromString(TypeStr);
					})
				]
			];
	}
	else if (ColumnName == "Value")
	{
		return SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SNew(SWidgetSwitcher)
				.WidgetIndex_Lambda([this]()
				{
					return Item->PropertyType == ELiveConfigPropertyType::Bool ? 1 : 0;
				})
				+ SWidgetSwitcher::Slot()
				[
					SNew(SEditableTextBox)
					.Text_Lambda([this]() { return FText::FromString(Item->Value); })
					.OnVerifyTextChanged_Lambda([this](const FText& NewText, FText& OutError)
					{
						FString NewVal = NewText.ToString();
						if (Item->PropertyType == ELiveConfigPropertyType::Int)
						{
							if (NewVal.IsEmpty() || NewVal == TEXT("-")) return true;
							
							// Must be numeric
							if (!NewVal.IsNumeric())
							{
								OutError = LOCTEXT("ValueIntError", "Value must be a valid integer.");
								return false;
							}
							
							// Additionally check if it contains a decimal point which IsNumeric might allow depending on platform but we don't want for Int
							if (NewVal.Contains(TEXT(".")))
							{
								OutError = LOCTEXT("ValueIntDecimalError", "Integers cannot have decimal points.");
								return false;
							}
						}
						else if (Item->PropertyType == ELiveConfigPropertyType::Float)
						{
							if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) return true;
							if (!NewVal.IsNumeric())
							{
								OutError = LOCTEXT("ValueFloatError", "Value must be a valid number.");
								return false;
							}
						}
						return true;
					})
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
					{
						FString NewVal = NewText.ToString();
						if (Item->PropertyType == ELiveConfigPropertyType::Int)
						{
							if (NewVal.IsEmpty() || NewVal == TEXT("-")) { Item->Value = "0"; OnChanged.ExecuteIfBound(); return; }
							if (NewVal.IsNumeric()) { Item->Value = NewVal; OnChanged.ExecuteIfBound(); }
						}
						else if (Item->PropertyType == ELiveConfigPropertyType::Float)
						{
							if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) { Item->Value = "0"; OnChanged.ExecuteIfBound(); return; }
							if (NewVal.IsNumeric()) { Item->Value = NewVal; OnChanged.ExecuteIfBound(); }
						}
						else
						{
							Item->Value = NewVal;
							OnChanged.ExecuteIfBound();
						}
					})
				]
				+ SWidgetSwitcher::Slot()
				[
					SNew(SCheckBox)
					.IsChecked_Lambda([this]()
					{
						return Item->Value.ToBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
					{
						Item->Value = NewState == ECheckBoxState::Checked ? TEXT("true") : TEXT("false");
						OnChanged.ExecuteIfBound();
					})
				]
			];
	}
	else if (ColumnName == "Tags")
	{
		TSharedRef<SWidget> TagsWidget = SNew(SBox)
			.Padding(FMargin(4.0f, 0.0f))
			.VAlign(VAlign_Center)
			[
				SAssignNew(TagWrapBox, SWrapBox)
				.UseAllottedSize(true)
			];
		
		RefreshTags();
		return TagsWidget;
	}

	return SNullWidget::NullWidget;
}

void SLiveConfigPropertyRow::RefreshTags()
{
	if (TagWrapBox.IsValid())
	{
		TagWrapBox->ClearChildren();
		for (int32 i = 0; i < Item->Tags.Num(); ++i)
		{
			TagWrapBox->AddSlot()
			.Padding(2.0f)
			[
				SNew(SLiveConfigTagRow, Item, i, FSimpleDelegate::CreateSP(this, &SLiveConfigPropertyRow::OnTagChanged), GetTagColor)
			];
		}

		// Add the "Add Tag" button at the end
		TagWrapBox->AddSlot()
		.Padding(2.0f)
		[
			SNew(SComboButton)
			.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
			.HasDownArrow(false)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(4, 2))
			.OnGetMenuContent_Lambda([this]()
			{
				FMenuBuilder MenuBuilder(true, nullptr);
				TArray<FName> AvailableTags = KnownTagsAttribute.Get();
				
				bool bAnyTagsAdded = false;
				for (const FName& Tag : AvailableTags)
				{
					if (!Item->Tags.Contains(Tag))
					{
						MenuBuilder.AddMenuEntry(
							FText::FromName(Tag),
							FText::GetEmpty(),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, Tag]()
							{
								Item->Tags.Add(Tag);
								OnTagChanged();
							}))
						);
						bAnyTagsAdded = true;
					}
				}

				if (!bAnyTagsAdded)
				{
					MenuBuilder.AddMenuEntry(
						LOCTEXT("NoTagsAvailable", "No more tags available"),
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(),
						NAME_None,
						EUserInterfaceActionType::None
					);
				}
				
				return MenuBuilder.MakeWidget();
			})
			.ButtonContent()
			[
				SNew(SBorder)
				.Padding(FMargin(8, 2))
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AddTagLabel", "+ Tag"))
					.Font(FAppStyle::GetFontStyle("NormalFont"))
					.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
				]
			]
		];
	}
}

void SLiveConfigPropertyRow::OnTagChanged()
{
	RefreshTags();
	OnChanged.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
