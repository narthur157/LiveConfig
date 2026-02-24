// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "PropertyManager/SLiveConfigPropertyRow.h"
#include "PropertyManager/SLiveConfigTagRow.h"
#include "PropertyManager/SLiveConfigTagPicker.h"
#include "LiveConfigJson.h"
#include "PropertyManager/SLiveConfigPropertyValueWidget.h"
#include "Editor.h"
#include "LiveConfigEditorLib.h"
#include "LiveConfigStyle.h"
#include "LiveConfigSystem.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/SMultiLineEditableText.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyRow);

const FName SLiveConfigPropertyRow::ColumnNames::Name("Name");
const FName SLiveConfigPropertyRow::ColumnNames::Description("Description");
const FName SLiveConfigPropertyRow::ColumnNames::Type("Type");
const FName SLiveConfigPropertyRow::ColumnNames::Value("Value");
const FName SLiveConfigPropertyRow::ColumnNames::Tags("Tags");
const FName SLiveConfigPropertyRow::ColumnNames::Actions("Actions");

SLiveConfigPropertyRow::SLiveConfigPropertyRow()
{
}

// ReSharper disable once CppPassValueParameterByConstReference
void SLiveConfigPropertyRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyTreeNode>
                                       InItem, int32 InIndex)
{
	Item = InItem;
	OnDeleteProperty = InArgs._OnDeleteProperty;
	OnAddPropertyAtFolder = InArgs._OnAddPropertyAtFolder;
	OnBulkTagFolder = InArgs._OnBulkTagFolder;
	OnIsNameDuplicate = InArgs._IsNameDuplicate;
	OnChanged = InArgs._OnChanged;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	OnNavigateDown = InArgs._OnNavigateDown;
	OnNavigateUp = InArgs._OnNavigateUp;
	OnNavigateValue = InArgs._OnNavigateValue;
	OnRequestScroll = InArgs._OnRequestScroll;
	OnAddNewTag = InArgs._OnAddNewTag;
	OnMouseDown = InArgs._OnMouseDown;
	
	SMultiColumnTableRow<TSharedRef<FLiveConfigPropertyTreeNode>>::Construct(
		SPropertyRowParent::FArguments()
		.Style(FLiveConfigStyle::Get(), "LiveConfig.TableRow")
		.Padding(0), 
		InOwnerTable);

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
		if (ValueWidget.IsValid())
		{
			ValueWidget->RequestFocus();
			bNeedsValueFocus = false;
		}
	}
}

bool SLiveConfigPropertyRow::IsReadOnly() const
{
	if (Item.IsValid())
	{
		TSharedPtr<FLiveConfigPropertyTreeNode> ParentNode = Item->Parent.Pin();
		if (ParentNode.IsValid() && ParentNode->IsStruct())
		{
			return true;
		}
	}
	return false;
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
		return (Item->IsProperty() || Item->IsStruct()) ? GenerateDescriptionColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Type)
	{
		return (Item->IsProperty() || Item->IsStruct()) ? GenerateTypeColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Value)
	{
		return (Item->IsProperty() || Item->IsStruct()) ? GenerateValueColumnWidget() : SNullWidget::NullWidget;
	}
	else if (ColumnName == ColumnNames::Tags)
	{
		return (Item->IsProperty() || Item->IsStruct()) ? GenerateTagsColumnWidget() : SNullWidget::NullWidget;
	}

	return SNullWidget::NullWidget;
}

FReply SLiveConfigPropertyRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = SMultiColumnTableRow<TSharedRef<FLiveConfigPropertyTreeNode>>::OnMouseButtonDown(MyGeometry, MouseEvent);
	
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		OnMouseDown.ExecuteIfBound();
	}
	
	return Reply;
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateActionsColumnWidget()
{
	if (IsReadOnly())
	{
		return SNullWidget::NullWidget;
	}

	if (!Item->PropertyDefinition.IsValid())
	{
		return SNew(SBox)
			.HeightOverride(RowHeight)
			.Padding(2.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.IsFocusable(false)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(4, 2))
					.OnClicked_Lambda([this]()
					{
						OnAddPropertyAtFolder.ExecuteIfBound(Item->FullPath);
						return FReply::Handled();
					})
					.ToolTipText(LOCTEXT("AddPropertyToFolder", "Add property to this folder"))
					[
						SNew(SBorder)
						.Padding(FMargin(8, 2))
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AddPropertyLabel", "+ Property"))
							.Font(FAppStyle::GetFontStyle("NormalFont"))
							.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SComboButton)
					.IsFocusable(false)
					.HasDownArrow(false)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ContentPadding(FMargin(4, 2))
 				.OnGetMenuContent_Lambda([this]()
 				{
 					return SNew(SLiveConfigTagPicker)
 						.OnTagSelected_Lambda([this](FName InTag)
 						{
 							OnBulkTagFolder.ExecuteIfBound(Item->FullPath, InTag);
 						});
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
			];
	}

	return SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(2.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SComboButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.HasDownArrow(false)
				.ContentPadding(FMargin(4.0f, 2.0f))
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
				.OnGetMenuContent_Lambda([this]()
				{
					return SNew(SLiveConfigTagPicker)
						.TagVisibilityFilter([this](FName InTag)
						{
							return !Item->PropertyDefinition->Tags.Contains(InTag);
						})
						.OnTagSelected_Lambda([this](FName InTag)
						{
							Item->PropertyDefinition->Tags.Add(InTag);
							OnTagChanged();
						})
						.OnAddNewTag_Lambda([this](FName NewTag)
						{
							OnAddNewTag.ExecuteIfBound();
						});
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("FindUsagesTooltip", "Find usages of this property in assets (Blueprints, etc.)"))
				.OnClicked(this, &SLiveConfigPropertyRow::OnFindUsages)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Search"))
				]
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(2)
			[
				SNew(SButton)
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("EditDescriptionToolTip", "Edit Description"))
				.OnClicked(this, &SLiveConfigPropertyRow::HandleEditDescription)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Edit"))
					.DesiredSizeOverride(FVector2D(16, 16))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.IsFocusable(false)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.Visibility_Lambda([this]()
				{
					// Hide delete button for struct sub-properties
					if (Item.IsValid())
					{
						TSharedPtr<FLiveConfigPropertyTreeNode> Parent = Item->Parent.Pin();
						if (Parent.IsValid() && Parent->IsStruct())
						{
							return EVisibility::Collapsed;
						}
					}
					return EVisibility::Visible;
				})
				.OnClicked_Lambda([this]()
				{
					if (Item->IsStruct())
					{
						DeleteSubProperties();
					}
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
				.Visibility_Lambda([this]()
				{
					return Item->Children.Num() > 0 ? EVisibility::Visible : EVisibility::Hidden;
				})
			]
		];

	if (Item->IsProperty() || Item->IsStruct())
	{
		NameContent->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SAssignNew(NameTextBox, SEditableTextBox)
			.IsReadOnly(this, &SLiveConfigPropertyRow::IsReadOnly)
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

		NameContent->SetToolTipText(TAttribute<FText>::CreateLambda([this]() { return FText::FromString(Item->PropertyDefinition->Description); }));
	}
	else
	{
		NameContent->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
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
	if (!Item->PropertyDefinition.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	static TArray<TSharedPtr<ELiveConfigPropertyType>> TypeOptions;
	if (TypeOptions.Num() == 0)
	{
		TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::String));
		TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Int));
		TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Float));
		TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Bool));
		TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Struct));
	}

	return SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 0.0f))
		.VAlign(VAlign_Center)
		[
			SNew(SComboBox<TSharedPtr<ELiveConfigPropertyType>>)
			.IsFocusable(false)
			.OptionsSource(&TypeOptions)
			.IsEnabled_Lambda([this]() { return !IsReadOnly(); })
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
					
					bool bOldWasStruct = Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Struct;

					Item->PropertyDefinition->PropertyType = *NewType;
					switch (Item->PropertyDefinition->PropertyType)
					{
					case ELiveConfigPropertyType::String:
					case ELiveConfigPropertyType::Struct:
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

					if (bOldWasStruct && Item->PropertyDefinition->PropertyType != ELiveConfigPropertyType::Struct)
					{
						DeleteSubProperties();
						OnRequestScroll.ExecuteIfBound(Item->PropertyDefinition->PropertyName);
					}
					else if (!bOldWasStruct && Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Struct)
					{
						OnRequestScroll.ExecuteIfBound(Item->PropertyDefinition->PropertyName);
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
	if (!Item->PropertyDefinition.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	return SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 0.0f))
		.VAlign(VAlign_Center)
		[
			SAssignNew(ValueWidget, SLiveConfigPropertyValueWidget)
			.Value_Lambda([this]() { return Item->PropertyDefinition->Value; })
			.PropertyType_Lambda([this]() { return Item->PropertyDefinition->PropertyType; })
			.OnValueChanged_Lambda([this](const FString& NewValue)
			{
				TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Item->PropertyDefinition);
				Item->PropertyDefinition->Value = NewValue;
				
				if (OldDef->PropertyType == ELiveConfigPropertyType::Struct && OldDef->Value != NewValue)
				{
					DeleteSubProperties();
				}

				if (Item->PropertyDefinition->PropertyType == ELiveConfigPropertyType::Struct)
				{
					UScriptStruct* Struct = FindFirstObject<UScriptStruct>(*NewValue);
					if (Struct)
					{
						GenerateSubPropertiesForStruct(Struct);
					}
				}

				OnChanged.ExecuteIfBound(OldDef, Item->PropertyDefinition, ELiveConfigPropertyChangeType::Value);
			})
		];
}

TSharedRef<SWidget> SLiveConfigPropertyRow::GenerateTagsColumnWidget()
{
	if (!Item->PropertyDefinition.IsValid())
	{
		return SNullWidget::NullWidget;
	}

	TSharedRef<SWidget> TagsWidget = SNew(SBox)
		.HeightOverride(RowHeight)
		.Padding(FMargin(4.0f, 4.0f))
		.VAlign(VAlign_Top)
		[
			SAssignNew(TagScrollBox, SScrollBox)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.Orientation(Orient_Horizontal)
		];
	
	RefreshTags();
	return TagsWidget;
}

void SLiveConfigPropertyRow::GenerateSubPropertiesForStruct(const UScriptStruct* Struct)
{
	UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct called for struct: %s"), Struct ? *Struct->GetName() : TEXT("None"));

	if (!Struct || (!Item->IsProperty() && !Item->IsStruct()))
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("GenerateSubPropertiesForStruct: Invalid struct or item type (IsProperty: %d, IsStruct: %d)"), Item->IsProperty(), Item->IsStruct());
		return;
	}

	ULiveConfigSystem& System = ULiveConfigSystem::Get();

	FString Prefix = Item->PropertyDefinition->PropertyName.ToString();
	UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct: Using prefix: %s"), *Prefix);
	
	if (Prefix.IsEmpty() || Prefix.EndsWith(TEXT(".")))
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("GenerateSubPropertiesForStruct: Prefix is empty or ends with dot, skipping generation"));
		return;
	}

	bool bChanged = false;

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Prop = *It;
		FString FullPropName = Prefix + TEXT(".") + Prop->GetAuthoredName();
		FLiveConfigProperty ConfigProp(FullPropName);

		if (System.PropertyDefinitions.Contains(ConfigProp))
		{
			UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct: Property %s already exists, skipping"), *FullPropName);
			continue;
		}

		ELiveConfigPropertyType PropType = ELiveConfigPropertyType::String;
		FString DefaultValue = "";

		if (CastField<FDoubleProperty>(Prop) || CastField<FFloatProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Float;
			DefaultValue = "0";
		}
		else if (CastField<FIntProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Int;
			DefaultValue = "0";
		}
		else if (CastField<FBoolProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Bool;
			DefaultValue = "false";
		}
		else if (CastField<FStrProperty>(Prop) || CastField<FNameProperty>(Prop) || CastField<FTextProperty>(Prop) || CastField<FEnumProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::String;
			DefaultValue = "";
		}
		else
		{
			UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct: Skipping unsupported property type for %s"), *Prop->GetName());
			continue;
		}

		FLiveConfigPropertyDefinition NewDef;
		NewDef.PropertyName = ConfigProp;
		NewDef.PropertyType = PropType;
		NewDef.Value = DefaultValue;
		NewDef.Tags = Item->PropertyDefinition->Tags; // Inherit tags from parent struct property

		UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct: Adding property %s"), *FullPropName);
		ULiveConfigSystem::Get().SavePropertyDeferred(NewDef);
		
		bChanged = true;
	}

	if (bChanged)
	{
		UE_LOG(LogLiveConfig, Log, TEXT("GenerateSubPropertiesForStruct: Refreshing system and UI"));
		System.RebuildConfigCache();
		OnRequestRefresh.ExecuteIfBound();
	}
}

void SLiveConfigPropertyRow::DeleteSubProperties()
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();

	FString Prefix = Item->PropertyDefinition->PropertyName.ToString() + TEXT(".");
	TArray<FLiveConfigProperty> PropertiesToDelete;

	for (auto& Pair : System.PropertyDefinitions)
	{
		if (Pair.Key.ToString().StartsWith(Prefix))
		{
			PropertiesToDelete.Add(Pair.Key);
		}
	}

	if (PropertiesToDelete.Num() > 0)
	{
		UE_LOG(LogLiveConfig, Log, TEXT("DeleteSubProperties: Deleting %d sub-properties for prefix %s"), PropertiesToDelete.Num(), *Prefix);
		for (const FLiveConfigProperty& Prop : PropertiesToDelete)
		{
			System.PropertyDefinitions.Remove(Prop);
			if (JsonSystem)
			{
				JsonSystem->DeletePropertyFile(Prop.GetName());
			}
		}

		System.RebuildConfigCache();
		OnRequestRefresh.ExecuteIfBound();
	}
}

void SLiveConfigPropertyRow::OnAddNewTagClicked()
{
	OnAddNewTag.ExecuteIfBound();
}

FReply SLiveConfigPropertyRow::OnFindUsages()
{
	if (Item->PropertyDefinition.IsValid())
	{
		FName PropertyName = Item->PropertyDefinition->PropertyName.GetName();
		
		TArray<FName> RelatedNames;
		ULiveConfigEditorLib::GetRelatedPropertyNames(PropertyName, RelatedNames);

		TArray<FAssetIdentifier> AssetIdentifiers;
		for (const FName& Name : RelatedNames)
		{
			AssetIdentifiers.Add(FAssetIdentifier(FLiveConfigProperty::StaticStruct(), Name));
		}

		FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
	}
	return FReply::Handled();
}

FReply SLiveConfigPropertyRow::HandleEditDescription()
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
}

void SLiveConfigPropertyRow::RefreshTags()
{
	if (TagScrollBox.IsValid())
	{
		TagScrollBox->ClearChildren();
		for (int32 i = 0; i < Item->PropertyDefinition->Tags.Num(); ++i)
		{
			TagScrollBox->AddSlot()
			.Padding(2.0f)
			[
				SNew(SLiveConfigTagRow, Item->PropertyDefinition, i, FSimpleDelegate::CreateSP(this, &SLiveConfigPropertyRow::OnTagChanged), IsReadOnly())
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

#undef LOCTEXT_NAMESPACE
