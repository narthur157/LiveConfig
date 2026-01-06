#include "SLiveConfigPropertyManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "LiveConfigEditorSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "LiveConfigGameSettings.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

class SLiveConfigPropertyRow;

class SLiveConfigTagRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigTagRow) {}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex, FSimpleDelegate InOnChanged)
	{
		Item = InItem;
		Index = InIndex;
		OnChanged = InOnChanged;

		ChildSlot
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(2.0f)
			[
				SNew(SEditableTextBox)
				.Text(FText::FromName(Item->Tags[Index]))
				.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
				{
					Item->Tags[Index] = FName(*NewText.ToString());
					OnChanged.ExecuteIfBound();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					Item->Tags.RemoveAt(Index);
					OnChanged.ExecuteIfBound();
					return FReply::Handled();
				})
				[
					SNew(SImage).Image(FAppStyle::GetBrush("Icons.X"))
				]
			]
		];
	}

private:
	TSharedPtr<FLiveConfigPropertyDefinition> Item;
	int32 Index;
	FSimpleDelegate OnChanged;
};

class SLiveConfigPropertyRow : public SMultiColumnTableRow<TSharedPtr<FLiveConfigPropertyDefinition>>
{
public:
	DECLARE_DELEGATE_OneParam(FOnDeleteProperty, TSharedPtr<FLiveConfigPropertyDefinition>);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FIsNameDuplicate, FName);

	SLATE_BEGIN_ARGS(SLiveConfigPropertyRow) {}
		SLATE_EVENT(FOnDeleteProperty, OnDeleteProperty);
		SLATE_EVENT(FIsNameDuplicate, IsNameDuplicate);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyDefinition> InItem)
	{
		Item = InItem;
		OnDeleteProperty = InArgs._OnDeleteProperty;
		OnIsNameDuplicate = InArgs._IsNameDuplicate;
		SMultiColumnTableRow<TSharedPtr<FLiveConfigPropertyDefinition>>::Construct(FSuperRowType::FArguments(), InOwnerTable);
		RefreshTags();
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
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
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromName(Item->PropertyName.GetName()))
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type) { Item->PropertyName = FLiveConfigProperty(FName(*NewText.ToString())); })
					.ForegroundColor_Lambda([this]()
					{
						if (OnIsNameDuplicate.IsBound() && OnIsNameDuplicate.Execute(Item->PropertyName.GetName()))
						{
							return FSlateColor(FLinearColor::Red);
						}
						return FSlateColor::UseForeground();
					})
					.ToolTipText_Lambda([this]()
					{
						if (OnIsNameDuplicate.IsBound() && OnIsNameDuplicate.Execute(Item->PropertyName.GetName()))
						{
							return LOCTEXT("DuplicateNameError", "Duplicate property name!");
						}
						return FText::GetEmpty();
					})
				];
		}
		else if (ColumnName == "Description")
		{
			return SNew(SBox)
				.Padding(2.0f)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(Item->Description))
					.OnTextChanged_Lambda([this](const FText& NewText) { Item->Description = NewText.ToString(); })
				];
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
				.Padding(2.0f)
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
				.Padding(2.0f)
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
						.OnTextChanged_Lambda([this](const FText& NewText)
						{
							FString NewVal = NewText.ToString();
							if (Item->PropertyType == ELiveConfigPropertyType::Int)
							{
								if (NewVal.IsEmpty() || NewVal == TEXT("-")) { Item->Value = NewVal; return; }
								if (NewVal.IsNumeric()) { Item->Value = NewVal; }
							}
							else if (Item->PropertyType == ELiveConfigPropertyType::Float)
							{
								if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) { Item->Value = NewVal; return; }
								if (NewVal.IsNumeric()) { Item->Value = NewVal; }
							}
							else
							{
								Item->Value = NewVal;
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
						})
					]
				];
		}
		else if (ColumnName == "Tags")
		{
			return SNew(SBox)
				.Padding(2.0f)
				[
					SAssignNew(TagsBox, SWrapBox)
					.UseAllottedSize(true)
				];
		}

		return SNullWidget::NullWidget;
	}

	void RefreshTags()
	{
		if (TagsBox.IsValid())
		{
			TagsBox->ClearChildren();
			for (int32 i = 0; i < Item->Tags.Num(); ++i)
			{
				TagsBox->AddSlot()
				.Padding(2.0f)
				[
					SNew(SBox)
					.WidthOverride(120.0f)
					[
						SNew(SLiveConfigTagRow, Item, i, FSimpleDelegate::CreateSP(this, &SLiveConfigPropertyRow::RefreshTags))
					]
				];
			}

			// Add the "Add Tag" button at the end
			TagsBox->AddSlot()
			.Padding(2.0f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.OnClicked_Lambda([this]()
				{
					Item->Tags.Add(NAME_None);
					RefreshTags();
					return FReply::Handled();
				})
				.ToolTipText(LOCTEXT("AddTag", "Add Tag"))
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
					.DesiredSizeOverride(FVector2D(14, 14))
				]
			];
		}
	}

private:
	TSharedPtr<FLiveConfigPropertyDefinition> Item;
	TSharedPtr<SWrapBox> TagsBox;
	FOnDeleteProperty OnDeleteProperty;
	FIsNameDuplicate OnIsNameDuplicate;
};

void SLiveConfigPropertyManager::Construct(const FArguments& InArgs)
{
	RefreshList();

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("AddProperty", "Add Property"))
				.OnClicked_Lambda([this]() { OnAddNewProperty(); return FReply::Handled(); })
			]
			+ SHorizontalBox::Slot()
			.Padding(5, 0)
			.FillWidth(1.0f)
			[
				SAssignNew(SearchBox, SSearchBox)
				.OnTextChanged(this, &SLiveConfigPropertyManager::OnFilterTextChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Save", "Save Changes"))
				.OnClicked(this, &SLiveConfigPropertyManager::OnSave)
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SAssignNew(PropertyListView, SListView<TSharedPtr<FLiveConfigPropertyDefinition>>)
			.ListItemsSource(&FilteredPropertyList)
			.OnGenerateRow(this, &SLiveConfigPropertyManager::OnGenerateRow)
			.HeaderRow(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("Name").DefaultLabel(LOCTEXT("NameColumn", "Name")).FillWidth(0.2f)
				+ SHeaderRow::Column("Description").DefaultLabel(LOCTEXT("DescriptionColumn", "Description")).FillWidth(0.25f)
				+ SHeaderRow::Column("Type").DefaultLabel(LOCTEXT("TypeColumn", "Type")).FillWidth(0.1f)
				+ SHeaderRow::Column("Value").DefaultLabel(LOCTEXT("ValueColumn", "Value")).FillWidth(0.2f)
				+ SHeaderRow::Column("Tags").DefaultLabel(LOCTEXT("TagsColumn", "Tags")).FillWidth(0.25f)
			)
		]
	];
}

TSharedRef<ITableRow> SLiveConfigPropertyManager::OnGenerateRow(TSharedPtr<FLiveConfigPropertyDefinition> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLiveConfigPropertyRow, OwnerTable, InItem)
		.OnDeleteProperty(this, &SLiveConfigPropertyManager::RemoveProperty)
		.IsNameDuplicate(this, &SLiveConfigPropertyManager::IsNameDuplicate);
}

void SLiveConfigPropertyManager::RefreshList()
{
	FullPropertyList.Empty();
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	for (auto& Pair : Settings->PropertyDefinitions)
	{
		FullPropertyList.Add(MakeShared<FLiveConfigPropertyDefinition>(Pair.Value));
	}

	FullPropertyList.Sort([](const TSharedPtr<FLiveConfigPropertyDefinition>& A, const TSharedPtr<FLiveConfigPropertyDefinition>& B)
	{
		return A->PropertyName.ToString() < B->PropertyName.ToString();
	});
	
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

void SLiveConfigPropertyManager::OnFilterTextChanged(const FText& InFilterText)
{
	FilteredPropertyList.Empty();
	FString FilterString = InFilterText.ToString();

	for (const auto& PropDef : FullPropertyList)
	{
		if (FilterString.IsEmpty() || 
			PropDef->PropertyName.ToString().Contains(FilterString) || 
			PropDef->Description.Contains(FilterString))
		{
			FilteredPropertyList.Add(PropDef);
		}
	}

	if (PropertyListView.IsValid())
	{
		PropertyListView->RequestListRefresh();
	}
}

void SLiveConfigPropertyManager::ScrollToProperty(FLiveConfigProperty Property)
{
	if (SearchBox.IsValid())
	{
		SearchBox->SetText(FText::GetEmpty());
	}

	for (const auto& PropDef : FullPropertyList)
	{
		if (PropDef->PropertyName == Property)
		{
			PropertyListView->SetSelection(PropDef);
			PropertyListView->RequestScrollIntoView(PropDef);
			break;
		}
	}
}

void SLiveConfigPropertyManager::OnAddNewProperty()
{
	TSharedPtr<FLiveConfigPropertyDefinition> NewProp = MakeShared<FLiveConfigPropertyDefinition>();
	NewProp->PropertyName = FLiveConfigProperty(FName(TEXT("NewProperty")));
	FullPropertyList.Add(NewProp);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

FReply SLiveConfigPropertyManager::OnSave()
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	Settings->PropertyDefinitions.Empty();
	for (const auto& PropDef : FullPropertyList)
	{
		Settings->PropertyDefinitions.Add(PropDef->PropertyName, *PropDef);
	}
	Settings->SaveConfig();

	// Notify system to refresh its base values
	if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
	{
		System->RefreshFromSettings();
	}

	return FReply::Handled();
}

void SLiveConfigPropertyManager::RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem)
{
	FullPropertyList.Remove(InItem);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

bool SLiveConfigPropertyManager::IsNameDuplicate(FName Name) const
{
	int32 Count = 0;
	for (const auto& PropDef : FullPropertyList)
	{
		if (PropDef->PropertyName.GetName() == Name)
		{
			Count++;
		}
	}
	return Count > 1;
}
