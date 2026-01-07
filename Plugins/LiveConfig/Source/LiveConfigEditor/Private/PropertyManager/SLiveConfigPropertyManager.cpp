#include "SLiveConfigPropertyManager.h"
#include "SLiveConfigPropertyRow.h"
#include "SLiveConfigTagRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "LiveConfigEditorSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "LiveConfigGameSettings.h"
#include "Misc/MessageDialog.h"
#include "Styling/SlateStyleMacros.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyManager);

void SLiveConfigPropertyManager::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigPropertyManager::Construct(const FArguments& InArgs)
{
	if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
	{
		System->OnPropertiesUpdated.AddSP(this, &SLiveConfigPropertyManager::RefreshList);
	}

	ChildSlot
	[
		SNew(SHorizontalBox)

		// Tag Filter Sidebar
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(5.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.0f))
			.Padding(FMargin(10, 10, 10, 10))
			[
				SNew(SBox)
				.WidthOverride(180.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.0f, 0.0f, 2.0f, 10.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TagsFilterTitle", "Tag Manager"))
						.Font(FAppStyle::GetFontStyle("BoldFont"))
						.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(TagFilterBox, SVerticalBox)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 10, 0, 0)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
						.HAlign(HAlign_Center)
						.OnClicked_Lambda([this]()
						{
							OnAddNewTag();
							return FReply::Handled();
						})
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0, 0, 4, 0)
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush("EditableComboBox.Add"))
								.ColorAndOpacity(FSlateColor::UseForeground())
							]
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("CreateNewTag", "Create New Tag"))
								.Font(FAppStyle::GetFontStyle("BoldFont"))
							]
						]
					]
				]
			]
		]

		// Main Content
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0f)
			[
				SNew(SBorder)
				.Padding(4.0f)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.5f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
						.Text(LOCTEXT("AddProperty", "+ Add Property"))
						.OnClicked_Lambda([this]() { OnAddNewProperty(); return FReply::Handled(); })
					]
					+ SHorizontalBox::Slot()
					.Padding(5, 0)
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(SearchBox, SSearchBox)
						.HintText(LOCTEXT("SearchPropertiesHint", "Search properties..."))
						.OnTextChanged(this, &SLiveConfigPropertyManager::OnFilterTextChanged)
					]
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
						.Style(FAppStyle::Get(), "TableView.Header")
						+ SHeaderRow::Column("Actions").DefaultLabel(LOCTEXT("ActionsColumn", "")).FixedWidth(30.0f)
					+ SHeaderRow::Column("Name")
					.DefaultLabel(LOCTEXT("NameColumn", "Name"))
					.FillWidth(0.35f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column("Type")
					.DefaultLabel(LOCTEXT("TypeColumn", "Type"))
					.FillWidth(0.15f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column("Value")
					.DefaultLabel(LOCTEXT("ValueColumn", "Value"))
					.FillWidth(0.2f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column("Tags")
					.DefaultLabel(LOCTEXT("TagsColumn", "Tags"))
					.FillWidth(0.3f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
				)
			]
		]
	];

	RefreshList();
}

TSharedRef<ITableRow> SLiveConfigPropertyManager::OnGenerateRow(TSharedPtr<FLiveConfigPropertyDefinition> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	int32 Index = FilteredPropertyList.IndexOfByKey(InItem);
	return SNew(SLiveConfigPropertyRow, OwnerTable, InItem, Index)
		.OnDeleteProperty(this, &SLiveConfigPropertyManager::RemoveProperty)
		.IsNameDuplicate(this, &SLiveConfigPropertyManager::IsNameDuplicate)
		.OnChanged(FSimpleDelegate::CreateSP(this, &SLiveConfigPropertyManager::Save))
		.GetTagColor(TFunction<FSlateColor(FName)>([this](FName InTag) { return GetTagColor(InTag); }))
		.KnownTags_Lambda([this]() { return KnownTags; });
}

void SLiveConfigPropertyManager::RefreshList()
{
	if (bIsSaving)
	{
		return;
	}

	FullPropertyList.Empty();
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	for (auto& Pair : Settings->PropertyDefinitions)
	{
		FullPropertyList.Add(MakeShared<FLiveConfigPropertyDefinition>(Pair.Value));
	}

	KnownTags = Settings->KnownTags;

	FullPropertyList.Sort([](const TSharedPtr<FLiveConfigPropertyDefinition>& A, const TSharedPtr<FLiveConfigPropertyDefinition>& B)
	{
		return A->PropertyName.ToString() < B->PropertyName.ToString();
	});
	
	UpdateAllTags();
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
	CheckForMissingTags();
}

void SLiveConfigPropertyManager::OnFilterTextChanged(const FText& InFilterText)
{
	FilteredPropertyList.Empty();
	FString FilterString = InFilterText.ToString();

	for (const auto& PropDef : FullPropertyList)
	{
		bool bPassesTextFilter = FilterString.IsEmpty() || 
			PropDef->PropertyName.ToString().Contains(FilterString) || 
			PropDef->Description.Contains(FilterString);

		bool bPassesTagFilter = SelectedTag.IsNone() || PropDef->Tags.Contains(SelectedTag);

		if (bPassesTextFilter && bPassesTagFilter)
		{
			FilteredPropertyList.Add(PropDef);
		}
	}

	if (PropertyListView.IsValid())
	{
		PropertyListView->RequestListRefresh();
	}
}

void SLiveConfigPropertyManager::UpdateAllTags()
{
	if (TagFilterBox.IsValid())
	{
		TagFilterBox->ClearChildren();

		auto CreateTagWidget = [this](FName TagName, FText DisplayName, bool bIsFilter)
		{
			int32 Count = GetTagCount(TagName);
			FText FullDisplayName = TagName.IsNone() ? DisplayName : FText::Format(LOCTEXT("TagWithCount", "{0} ({1})"), DisplayName, FText::AsNumber(Count));

			return SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(FMargin(0))
				.OnClicked_Lambda([this, TagName]()
				{
					OnTagFilterSelected(TagName);
					return FReply::Handled();
				})
				[
					SNew(SBorder)
					.Padding(FMargin(8, 4))
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.BorderBackgroundColor_Lambda([this, TagName]()
					{
						return SelectedTag == TagName ? FLinearColor(1.0f, 1.0f, 1.0f, 1.f) : FLinearColor::Transparent;
					})
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(0, 0, 8, 0)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor_Lambda([this, TagName]()
							{
								return TagName.IsNone() ? FLinearColor::Transparent : GetTagColor(TagName).GetSpecifiedColor();
							})
							[
								SNew(SBox)
								.WidthOverride(12.0f)
								.HeightOverride(12.0f)
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FullDisplayName)
							.ColorAndOpacity_Lambda([this, TagName]()
							{
								return SelectedTag == TagName ? FLinearColor::White : FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
							})
							.Font(SelectedTag == TagName ? FAppStyle::GetFontStyle("BoldFont") : FAppStyle::GetFontStyle("NormalFont"))
						]
					]
				];
		};

		TagFilterBox->AddSlot()
		.Padding(0, 2)
		[
			CreateTagWidget(NAME_None, LOCTEXT("AllTagsFilter", "All"), true)
		];

		for (const FName& PropertyTag : KnownTags)
		{
			TagFilterBox->AddSlot()
			.Padding(0, 2)
			[
				CreateTagWidget(PropertyTag, FText::FromName(PropertyTag), true)
			];
		}
	}
}

int32 SLiveConfigPropertyManager::GetTagCount(FName InTag) const
{
	if (InTag.IsNone()) return FullPropertyList.Num();
	
	int32 Count = 0;
	for (const auto& PropDef : FullPropertyList)
	{
		if (PropDef->Tags.Contains(InTag))
		{
			Count++;
		}
	}
	return Count;
}

void SLiveConfigPropertyManager::OnAddNewTag()
{
	TSharedPtr<SEditableTextBox> TagNameTextBox;

	SAssignNew(NewTagWindow, SWindow)
		.Title(LOCTEXT("CreateNewTagTitle", "Create New Tag"))
		.ClientSize(FVector2D(300, 100))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(10)
			.AutoHeight()
			[
				SAssignNew(TagNameTextBox, SEditableTextBox)
				.HintText(LOCTEXT("NewTagNameHint", "Enter tag name..."))
			]
			+ SVerticalBox::Slot()
			.Padding(10, 0, 10, 10)
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SButton)
				.Text(LOCTEXT("AddTagOk", "Add"))
				.OnClicked_Lambda([this, TagNameTextBox]()
				{
					FName NewTag = FName(*TagNameTextBox->GetText().ToString());
					if (!NewTag.IsNone() && !KnownTags.Contains(NewTag))
					{
						KnownTags.Add(NewTag);
						SaveKnownTags();
						UpdateAllTags();
					}
					
					if (NewTagWindow)
					{
						NewTagWindow->RequestDestroyWindow();
					}
					
					return FReply::Handled();
				})
			]
		];

	FSlateApplication::Get().AddWindow(NewTagWindow.ToSharedRef());
}

void SLiveConfigPropertyManager::CheckForMissingTags()
{
	TSet<FName> UsedTags;
	for (const auto& PropDef : FullPropertyList)
	{
		for (const FName& PropertyTag : PropDef->Tags)
		{
			UsedTags.Add(PropertyTag);
		}
	}

	TArray<FName> MissingTags;
	for (const FName& PropertyTag : UsedTags)
	{
		if (!KnownTags.Contains(PropertyTag))
		{
			MissingTags.Add(PropertyTag);
		}
	}

	if (MissingTags.Num() > 0)
	{
		FText Message = FText::Format(
			LOCTEXT("MissingTagsPrompt", "Found {0} tags used in properties that are not in the Known Tags list. Would you like to import them?\n\nMissing Tags: {1}"),
			FText::AsNumber(MissingTags.Num()),
			FText::FromString(FString::JoinBy(MissingTags, TEXT(", "), [](const FName& TagItem) { return TagItem.ToString(); }))
		);

		if (FMessageDialog::Open(EAppMsgType::YesNo, Message, LOCTEXT("ImportTagsTitle", "Import Missing Tags")) == EAppReturnType::Yes)
		{
			for (const FName& PropertyTag : MissingTags)
			{
				KnownTags.Add(PropertyTag);
			}
			SaveKnownTags();
			UpdateAllTags();
		}
	}
}

void SLiveConfigPropertyManager::SaveKnownTags()
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	Settings->KnownTags = KnownTags;
	Settings->SaveConfig();
	Settings->TryUpdateDefaultConfigFile();
}

void SLiveConfigPropertyManager::OnTagFilterSelected(FName InTag)
{
	SelectedTag = InTag;
	UpdateAllTags(); // Refresh colors/highlights
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

FSlateColor SLiveConfigPropertyManager::GetTagColor(FName InTag) const
{
	if (InTag.IsNone())
	{
		return FSlateColor(FLinearColor::Gray);
	}

	uint32 Hash = GetTypeHash(InTag.ToString());
	
	// Generate a color from hash
	float Hue = (Hash % 360);
	float Saturation = 0.8f;
	float Value = 0.6f;

	// Simple HSV to RGB
	auto HSVtoRGB = [](float h, float s, float v) -> FLinearColor
	{
		float c = v * s;
		float x = c * (1.0f - FMath::Abs(FMath::Fmod(h / 60.0f, 2.0f) - 1.0f));
		float m = v - c;
		float r, g, b;
		if (h < 60) { r = c; g = x; b = 0; }
		else if (h < 120) { r = x; g = c; b = 0; }
		else if (h < 180) { r = 0; g = c; b = x; }
		else if (h < 240) { r = 0; g = x; b = c; }
		else if (h < 300) { r = x; g = 0; b = c; }
		else { r = c; g = 0; b = x; }
		return FLinearColor(r + m, g + m, b + m, 1.0f);
	};

	return FSlateColor(HSVtoRGB(Hue, Saturation, Value));
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
	Save();
}

void SLiveConfigPropertyManager::Save()
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
		bIsSaving = true;
		System->RefreshFromSettings();
		bIsSaving = false;
	}
}

void SLiveConfigPropertyManager::RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem)
{
	FullPropertyList.Remove(InItem);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
	Save();
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
