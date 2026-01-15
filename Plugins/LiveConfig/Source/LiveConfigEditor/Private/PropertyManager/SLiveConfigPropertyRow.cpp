#include "SLiveConfigPropertyRow.h"
#include "SLiveConfigTagRow.h"
#include "Widgets/Views/STableViewBase.h"
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
#include "Widgets/Layout/SScrollBox.h"
#include "Styling/SlateStyleMacros.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Misc/ComparisonUtility.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyRow);

const FName SLiveConfigPropertyRow::ColumnNames::Name("Name");
const FName SLiveConfigPropertyRow::ColumnNames::Description("Description");
const FName SLiveConfigPropertyRow::ColumnNames::Type("Type");
const FName SLiveConfigPropertyRow::ColumnNames::Value("Value");
const FName SLiveConfigPropertyRow::ColumnNames::Tags("Tags");
const FName SLiveConfigPropertyRow::ColumnNames::Actions("Actions");

void SLiveConfigPropertyRow::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigPropertyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyTreeNode>
                                       InItem, int32 InIndex)
{
	Item = InItem;
	OnDeleteProperty = InArgs._OnDeleteProperty;
	OnAddPropertyAtFolder = InArgs._OnAddPropertyAtFolder;
	OnIsNameDuplicate = InArgs._IsNameDuplicate;
	OnChanged = InArgs._OnChanged;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	OnNavigateDown = InArgs._OnNavigateDown;
	OnNavigateUp = InArgs._OnNavigateUp;
	OnNavigateValue = InArgs._OnNavigateValue;
	OnAddNewTag = InArgs._OnAddNewTag;
	GetTagColor = InArgs._GetTagColor;
	KnownTagsAttribute.Assign(*this, InArgs._KnownTags);
	
	SMultiColumnTableRow<TSharedRef<FLiveConfigPropertyTreeNode>>::Construct(
		SPropertyRowParent::FArguments()
		.Padding(0), 
		InOwnerTable);

	if (!Item->IsProperty())
	{
		// Folder nodes get a distinct darker background
		SetBorderImage(FAppStyle::GetBrush("WhiteBrush"));
		SetBorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 1.0f));
	}
	else if (InIndex % 2 == 1)
	{
		// Use a subtle dark background for striping
		SetBorderImage(FAppStyle::GetBrush("WhiteBrush"));
		SetBorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.02f));
	}
	else
	{
		SetBorderImage(FAppStyle::GetBrush("NoBorder"));
	}

	// If the name is empty or ends with a dot, it's a new property, request focus
	if (Item->IsProperty() && (!Item->PropertyDefinition->PropertyName.IsValid() || Item->PropertyDefinition->PropertyName.ToString().EndsWith(TEXT(".")) || Item->bNeedsFocus))
	{
		bNeedsFocus = true;
		Item->bNeedsFocus = false;
	}

	bJustFinishedEnterCommit = false;
}

void SLiveConfigPropertyRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPropertyRowParent::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bJustFinishedEnterCommit)
	{
		bJustFinishedEnterCommit = false;
	}

	if (bNeedsFocus)
	{
		if (NameTextBox.IsValid())
		{
			TSharedRef<SEditableTextBox> NameTextBoxRef = NameTextBox.ToSharedRef();
			FSlateApplication::Get().SetKeyboardFocus(NameTextBoxRef, EFocusCause::SetDirectly);
			bNeedsFocus = false;
		}
	}

	if (bNeedsValueFocus)
	{
		if (ValueTextBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(ValueTextBox.ToSharedRef(), EFocusCause::SetDirectly);
			bNeedsValueFocus = false;
		}
		else if (ValueCheckBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(ValueCheckBox.ToSharedRef(), EFocusCause::SetDirectly);
			bNeedsValueFocus = false;
		}
	}
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == ColumnNames::Actions)
	{
		return GenerateActionsColumnWidget();
	}
	else if (ColumnName == ColumnNames::Name)
	{
		return GenerateNameColumnWidget();
	}
	else if (ColumnName == ColumnNames::Description)
	{
		return Item->IsProperty() ? GenerateDescriptionColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Type)
	{
		return Item->IsProperty() ? GenerateTypeColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Value)
	{
		return Item->IsProperty() ? GenerateValueColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Tags)
	{
		return Item->IsProperty() ? GenerateTagsColumnWidget() : SNullWidget::NullWidget;
	}

	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateActionsColumnWidget()
{
	if (!Item->IsProperty())
	{
		return SNew(SBox)
			.HeightOverride(RowHeight)
			.Padding(2.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					OnAddPropertyAtFolder.ExecuteIfBound(Item->FullPath);
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("AddPropertyToFolder", "Add property to this folder"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
					.DesiredSizeOverride(FVector2D(12, 12))
				]
			];
	}

	return SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(2.0f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(2.0f)
			.AutoWidth()
			[
				SNew(SComboButton)
				.IsFocusable(false)
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
						if (!Item->PropertyDefinition->Tags.Contains(Tag))
						{
							MenuBuilder.AddMenuEntry(
								FText::FromName(Tag),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, Tag]()
								{
									Item->PropertyDefinition->Tags.Add(Tag);
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
			]
			+ SHorizontalBox::Slot()
				.AutoWidth()
			[
				SNew(SButton)
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					OnDeleteProperty.ExecuteIfBound(Item->PropertyDefinition);
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("DeleteProperty", "Delete Property"))
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Icons.Delete"))
				]
			]
		];
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateNameColumnWidget()
{
	TSharedPtr<SHorizontalBox> NameContent;

	TSharedRef<SWidget> NameWidget = SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 4.0f))
		.VAlign(VAlign_Center)
		[
			SAssignNew(NameContent, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
		];

	if (Item->IsProperty())
	{
		NameContent->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SAssignNew(NameTextBox, SEditableTextBox)
			.Font(FAppStyle::GetFontStyle("BoldFont"))
			.ForegroundColor(FLinearColor::Black)
			.Text_Lambda([this]()
			{
				if (Item->PropertyDefinition.IsValid() && Item->PropertyDefinition->PropertyName.GetName().IsNone())
				{
					return FText::GetEmpty();
				}
				return FText::FromString(Item->FullPath);
			})
			.OnVerifyTextChanged_Lambda([](const FText& NewText, FText& OutError)
			{
				FString TextStr = NewText.ToString();
				if (TextStr.Contains(TEXT(" ")))
				{
					OutError = LOCTEXT("PropertyNameSpaceError", "Property names cannot contain spaces.");
					return false;
				}
				if (TextStr.EndsWith(TEXT(".")))
				{
					OutError = LOCTEXT("PropertyNameDotError", "Property names cannot end with a dot.");
					return false;
				}
				return true;
			})
			.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
			{
				// If we are already committing, ignore this event.
				if (bIsCommitting)
				{
					return;
				}

				// If it's a focus loss, but we just handled an Enter commit, ignore it.
				if (CommitType == ETextCommit::OnUserMovedFocus && bJustFinishedEnterCommit)
				{
					return;
				}

				if (CommitType == ETextCommit::Default)
				{
					return;
				}

				FString NewFullName = NewText.ToString();
				if (NewFullName == Item->PropertyDefinition->PropertyName.ToString())
				{
					if (CommitType == ETextCommit::OnEnter)
					{
						OnNavigateValue.ExecuteIfBound(Item);
					}
					return;
				}

				// If it's a focus loss and the name is empty or ends with a dot, 
				// don't try to commit it as it would just trigger a refresh and potentially re-focus.
				if (CommitType == ETextCommit::OnUserMovedFocus && (NewFullName.IsEmpty() || NewFullName.EndsWith(TEXT("."))))
				{
					return;
				}
				
				TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
				
				// Temporarily set the name on the Item so that the Refresh triggered by OnChanged
				// knows about the new name immediately.
				Item->PropertyDefinition->PropertyName = FLiveConfigProperty(FName(*NewFullName));
				Item->FullPath = NewFullName; // Update the path too as it's used in the lambda

				{
					TGuardValue<bool> CommitGuard(bIsCommitting, true);
					OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Name);
				}

				if (CommitType == ETextCommit::OnEnter)
				{
					bJustFinishedEnterCommit = true;
					OnNavigateValue.ExecuteIfBound(Item);
				}
			})
			.OnKeyDownHandler_Lambda([this](const FGeometry&, const FKeyEvent& InKeyEvent)
			{
				const FKey Key = InKeyEvent.GetKey();
				if (Key == EKeys::Down || Key == EKeys::Up)
				{
					(Key == EKeys::Down ? OnNavigateDown : OnNavigateUp).ExecuteIfBound(Item);
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			.ForegroundColor_Lambda([this]()
			{
				if (OnIsNameDuplicate.IsBound() && OnIsNameDuplicate.Execute(Item->PropertyDefinition->PropertyName.GetName()))
				{
					return FSlateColor(FLinearColor::Red);
				}
				return FSlateColor::UseForeground();
			})
		];

		NameContent->AddSlot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.IsFocusable(false)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("EditDescriptionToolTip", "Edit Description"))
			.OnClicked_Lambda([this]()
			{
				TSharedRef<SWindow> EditWindow = SNew(SWindow)
					.Title(LOCTEXT("EditDescriptionTitle", "Edit Description"))
					.ClientSize(FVector2D(400, 200))
					.SupportsMaximize(false)
					.SupportsMinimize(false);

				TSharedPtr<SMultiLineEditableText> DescriptionText;

				EditWindow->SetContent(
					SNew(SBox)
					.Padding(10)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SAssignNew(DescriptionText, SMultiLineEditableText)
								.Text(FText::FromString(Item->PropertyDefinition->Description))
								.AutoWrapText(true)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 10, 0, 0)
						.HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(0, 0, 5, 0)
							[
								SNew(SButton)
								.Text(LOCTEXT("OK", "OK"))
								.OnClicked_Lambda([this, EditWindow, DescriptionText]()
								{
									TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
									Item->PropertyDefinition->Description = DescriptionText->GetText().ToString();
									OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Description);
									EditWindow->RequestDestroyWindow();
									return FReply::Handled();
								})
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SButton)
								.Text(LOCTEXT("Cancel", "Cancel"))
								.OnClicked_Lambda([EditWindow]()
								{
									EditWindow->RequestDestroyWindow();
									return FReply::Handled();
								})
							]
						]
					]
				);

				FSlateApplication::Get().AddModalWindow(EditWindow, FSlateApplication::Get().FindBestParentWindowForDialogs(AsShared()));

				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Info"))
				.DesiredSizeOverride(FVector2D(12, 12))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];

		NameContent->SetToolTipText(TAttribute<FText>::CreateLambda([this]() { return FText::FromString(Item->PropertyDefinition->Description); }));
	}
	else
	{
		NameContent->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(FAppStyle::GetFontStyle("BoldFont"))
			.Text(FText::FromString(Item->FullPath))
		];
	}

	return NameWidget;
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateDescriptionColumnWidget()
{
	return SNullWidget::NullWidget;
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateTypeColumnWidget()
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
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 0.0f))
		.VAlign(VAlign_Center)
		[
			SNew(SComboBox<TSharedPtr<ELiveConfigPropertyType>>)
			.IsFocusable(false)
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
					TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
					Item->PropertyDefinition->PropertyType = *NewType;
					switch (Item->PropertyDefinition->PropertyType)
					{
					case ELiveConfigPropertyType::String:
						Item->PropertyDefinition->Value = "";
						break;
					case ELiveConfigPropertyType::Int:
					case ELiveConfigPropertyType::Float:
						Item->PropertyDefinition->Value = "0";
						break;
					case ELiveConfigPropertyType::Bool:
						Item->PropertyDefinition->Value = "false";
						break;
					}
					OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Type);
				}
			})
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					FString TypeStr = StaticEnum<ELiveConfigPropertyType>()->GetNameStringByValue((int64)Item->PropertyDefinition->PropertyType);
					return FText::FromString(TypeStr);
				})
			]
		];
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateValueColumnWidget()
{
	return SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 0.0f))
		.VAlign(VAlign_Center)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]()
			{
				return Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Bool ? 1 : 0;
			})
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(ValueTextBox, SEditableTextBox)
				.Text_Lambda([this]() { return FText::FromString(Item->PropertyDefinition->Value); })
				.OnVerifyTextChanged(this, &SLiveConfigPropertyRow::VerifyValueText)
				.OnTextCommitted(this, &SLiveConfigPropertyRow::ValueTextCommitted)
			]
			+ SWidgetSwitcher::Slot()
			[
				SAssignNew(ValueCheckBox, SCheckBox)
				.IsChecked_Lambda([this]()
				{
					return Item->PropertyDefinition->Value.ToBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
					Item->PropertyDefinition->Value = NewState == ECheckBoxState::Checked ? TEXT("true") : TEXT("false");
					OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value);
				})
			]
		];
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateTagsColumnWidget()
{
	TSharedRef<SWidget> TagsWidget = SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 4.0f))
		.VAlign(VAlign_Top)
		[
			SNew(SScrollBox)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.Orientation(Orient_Horizontal)
			+ SScrollBox::Slot()
			[
				SAssignNew(TagWrapBox, SWrapBox)
				.UseAllottedSize(true)
			]
			+ SScrollBox::Slot()
			.Padding(4.0f, 0.0f)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.HasDownArrow(false)
				.ContentPadding(FMargin(4.0f, 2.0f))
				.ButtonContent()
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
					.DesiredSizeOverride(FVector2D(12.0f, 12.0f))
				]
				.OnGetMenuContent_Lambda([this]()
				{
					FMenuBuilder MenuBuilder(true, nullptr);
					
					TArray<FName> Tags = KnownTagsAttribute.Get();
					Tags.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

					for (const FName& Tag : Tags)
					{
						if (!Item->PropertyDefinition->Tags.Contains(Tag))
						{
							MenuBuilder.AddMenuEntry(
								FText::FromName(Tag),
								FText::GetEmpty(),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, Tag]()
								{
									Item->PropertyDefinition->Tags.Add(Tag);
									OnTagChanged();
								}))
							);
						}
					}

					if (OnAddNewTag.IsBound())
					{
						MenuBuilder.AddMenuSeparator();
						MenuBuilder.AddMenuEntry(
							LOCTEXT("AddNewTagMenu", "Add New Tag..."),
							FText::GetEmpty(),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"),
							FUIAction(FExecuteAction::CreateSP(this, &SLiveConfigPropertyRow::OnAddNewTagClicked))
						);
					}

					return MenuBuilder.MakeWidget();
				})
			]
		];
	
	RefreshTags();
	return TagsWidget;
}

void SLiveConfigPropertyRow::OnAddNewTagClicked()
{
	OnAddNewTag.ExecuteIfBound();
}

void SLiveConfigPropertyRow::RefreshTags()
{
	if (TagWrapBox.IsValid())
	{
		TagWrapBox->ClearChildren();
		for (int32 i = 0; i < Item->PropertyDefinition->Tags.Num(); ++i)
		{
			TagWrapBox->AddSlot()
			.Padding(2.0f)
			[
				SNew(SLiveConfigTagRow, Item->PropertyDefinition, i, FSimpleDelegate::CreateSP(this, &SLiveConfigPropertyRow::OnTagChanged), GetTagColor)
			];
		}
	}
}

void SLiveConfigPropertyRow::OnTagChanged()
{
	TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
	RefreshTags();
	Invalidate(EInvalidateWidgetReason::Layout);
	OnRequestRefresh.ExecuteIfBound();
	OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Tags);
}

bool SLiveConfigPropertyRow::VerifyValueText(const FText& NewText, FText& OutError)
{
	FString NewVal = NewText.ToString();
	if (Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Int)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-"))
		{
			return true;
		}

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
	else if (Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Float)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) return true;
		if (!NewVal.IsNumeric())
		{
			OutError = LOCTEXT("ValueFloatError", "Value must be a valid number.");
			return false;
		}
	}
	return true;
}

void SLiveConfigPropertyRow::ValueTextCommitted(const FText& NewText, ETextCommit::Type CommitType)
{
	if (bIsCommitting)
	{
		return;
	}

	if (CommitType == ETextCommit::OnUserMovedFocus && bJustFinishedEnterCommit)
	{
		return;
	}

	if (CommitType == ETextCommit::Default)
	{
		return;
	}
	
	FString NewVal = NewText.ToString();
	if (NewVal == Item->PropertyDefinition->Value)
	{
		return;
	}
	
	TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
	
	TGuardValue<bool> CommitGuard(bIsCommitting, true);
	
	if (CommitType == ETextCommit::OnEnter)
	{
		bJustFinishedEnterCommit = true;
	}

	if (Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Int)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-"))
		{
			Item->PropertyDefinition->Value = "0"; 
			OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value); 
			return;
		}
		if (NewVal.IsNumeric())
		{
			Item->PropertyDefinition->Value = NewVal; 
			OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value);
		}
	}
	else if (Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Float)
	{
		if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-."))
		{
			Item->PropertyDefinition->Value = "0"; 
			OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value); 
			return;
		}
		
		if (NewVal.IsNumeric())
		{
			float FloatVal = FCString::Atof(*NewVal);
			Item->PropertyDefinition->Value = FString::SanitizeFloat(FloatVal); 
			OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value);
		}
	}
	else
	{
		Item->PropertyDefinition->Value = NewVal;
		OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value);
	}
}

#undef LOCTEXT_NAMESPACE
