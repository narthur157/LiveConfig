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
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SScrollBox.h"
#include "LiveConfigGameSettings.h"
#include "LiveConfigJson.h"
#include "LiveConfigSystem.h"
#include "Misc/MessageDialog.h"
#include "Misc/ComparisonUtility.h"

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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0, 0, 4, 0)
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
								]
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CreateNewTag", "Add Tag"))
									.Font(FAppStyle::GetFontStyle("BoldFont"))
									.ToolTipText(LOCTEXT("CreateNewTagToolTip", "Create a new tag"))
								]
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.Padding(2.0f)
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(FMargin(4, 2))
								.IsEnabled_Lambda([this]() { return !SelectedTag.IsNone() && SelectedTag != LiveConfigTags::FromCurveTable; })
								.OnClicked_Lambda([this]()
								{
									RemoveTag(SelectedTag);
									return FReply::Handled();
								})
								.ToolTipText(LOCTEXT("DeleteSelectedTagToolTip", "Delete the currently selected tag"))
								[
									SNew(SImage)
									.Image(FAppStyle::GetBrush("Icons.Delete"))
								]
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
						.OnClicked_Lambda([this]()
						{
							OnAddNewProperty(); 
							return FReply::Handled();
						})
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
				SAssignNew(PropertyTreeView, STreeView<TSharedRef<FLiveConfigPropertyTreeNode>>)
				.TreeItemsSource(&RootNodes)
				.OnGenerateRow(this, &SLiveConfigPropertyManager::OnGenerateRow)
				.OnGetChildren(this, &SLiveConfigPropertyManager::OnGetChildren)
				.HeaderRow(
					SNew(SHeaderRow)
					.Style(FAppStyle::Get(), "TableView.Header")
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Name)
					.DefaultLabel(LOCTEXT("NameColumn", "Name"))
					.FillWidth(0.35f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Type)
					.DefaultLabel(LOCTEXT("TypeColumn", "Type"))
					.FillWidth(0.1f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Value)
					.DefaultLabel(LOCTEXT("ValueColumn", "Value"))
					.FillWidth(0.2f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Tags)
					.DefaultLabel(LOCTEXT("TagsColumn", "Tags"))
					.FillWidth(0.35f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Actions)
					.DefaultLabel(LOCTEXT("ActionsColumn", ""))
					.FixedWidth(120.0f)
				)
			]
		]
	];

	// RefreshList(); // Removed to allow Tick to handle it once geometry is valid
}

void SLiveConfigPropertyManager::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bNeedsInitialRefresh)
	{
		RefreshList();
		bNeedsInitialRefresh = false;
	}

	if (PendingScrollProperty.IsValid())
	{
		ScrollToProperty(PendingScrollProperty);
		PendingScrollProperty = FLiveConfigProperty();
	}
}

TSharedRef<ITableRow> SLiveConfigPropertyManager::OnGenerateRow(TSharedRef<FLiveConfigPropertyTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLiveConfigPropertyRow, OwnerTable, InItem, 0)
		.OnDeleteProperty(this, &SLiveConfigPropertyManager::RemoveProperty)
		.OnAddPropertyAtFolder(this, &SLiveConfigPropertyManager::OnAddPropertyAtFolder)
		.IsNameDuplicate(this, &SLiveConfigPropertyManager::IsNameDuplicate)
		.OnChanged(this, &SLiveConfigPropertyManager::OnPropertyRowChanged)
		.OnRequestRefresh_Lambda([this]() { if (PropertyTreeView.IsValid()) PropertyTreeView->RequestTreeRefresh(); })
		.OnNavigateDown_Lambda([this](TSharedPtr<FLiveConfigPropertyTreeNode> Item)
		{
			NavigateToProperty(Item, 1);
		})
		.OnNavigateUp_Lambda([this](TSharedPtr<FLiveConfigPropertyTreeNode> Item)
		{
			NavigateToProperty(Item, -1);
		})
		.OnNavigateValue_Lambda([this](TSharedPtr<FLiveConfigPropertyTreeNode> Item)
		{
			// Defer to next tick to ensure any tree refresh has stabilized
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, Item](float)
			{
				if (PropertyTreeView.IsValid())
				{
					TSharedPtr<ITableRow> Row = PropertyTreeView->WidgetFromItem(Item.ToSharedRef());
					if (Row.IsValid())
					{
						TSharedRef<SWidget> RowWidget = Row->AsWidget();
						SLiveConfigPropertyRow* PropRow = static_cast<SLiveConfigPropertyRow*>(&RowWidget.Get());
						if (PropRow)
						{
							PropRow->RequestValueFocus();
						}
					}
				}
				return false;
			}));
		})
		.GetTagColor(TFunction<FSlateColor(FName)>([this](FName InTag) { return GetTagColor(InTag); }))
		.KnownTags_Lambda([this]() { return KnownTags; });
}

void SLiveConfigPropertyManager::OnGetChildren(TSharedRef<FLiveConfigPropertyTreeNode> InItem, TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutChildren)
{
	OutChildren.Append(InItem->Children);
}

void SLiveConfigPropertyManager::RefreshList()
{
	RawPropertyList.Empty();
	ULiveConfigSystem* System = ULiveConfigSystem::Get();
	for (auto& Pair : System->PropertyDefinitions)
	{
		RawPropertyList.Add(MakeShared<FLiveConfigPropertyDefinition>(Pair.Value));
	}

	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	KnownTags = Settings->KnownTags;

	RawPropertyList.Sort([](const TSharedPtr<FLiveConfigPropertyDefinition>& A, const TSharedPtr<FLiveConfigPropertyDefinition>& B)
	{
		return UE::ComparisonUtility::CompareNaturalOrder(A->PropertyName.ToString(), B->PropertyName.ToString()) < 0;
	});
	
	UpdateAllTags();
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

void SLiveConfigPropertyManager::OnFilterTextChanged(const FText& InFilterText)
{
	TSet<FString> ExpandedPaths;
	if (PropertyTreeView.IsValid())
	{
		TSet<TSharedRef<FLiveConfigPropertyTreeNode>> ExpandedItems;
		PropertyTreeView->GetExpandedItems(ExpandedItems);
		for (const auto& ItemPtr : ExpandedItems)
		{
			ExpandedPaths.Add(ItemPtr->FullPath);
		}
	}

	RootNodes.Empty();
	FString FilterString = InFilterText.ToString();

	TMap<FString, TSharedPtr<FLiveConfigPropertyTreeNode>> FolderMap;
	TFunction<TSharedPtr<FLiveConfigPropertyTreeNode>(FString)> GetOrCreateFolder;
	GetOrCreateFolder = [&](FString FolderPath) -> TSharedPtr<FLiveConfigPropertyTreeNode>
	{
		if (TSharedPtr<FLiveConfigPropertyTreeNode>* FoundFolder = FolderMap.Find(FolderPath))
		{
			return *FoundFolder;
		}

		TSharedRef<FLiveConfigPropertyTreeNode> NewFolder = MakeShared<FLiveConfigPropertyTreeNode>();
		NewFolder->FullPath = FolderPath;
		
		int32 LastDot;
		if (FolderPath.FindLastChar('.', LastDot))
		{
			NewFolder->DisplayName = FolderPath.RightChop(LastDot + 1);
			FString ParentPath = FolderPath.Left(LastDot);
			TSharedPtr<FLiveConfigPropertyTreeNode> ParentFolder = GetOrCreateFolder(ParentPath);
			if (ParentFolder.IsValid())
			{
				ParentFolder->Children.Add(NewFolder);
				NewFolder->Parent = ParentFolder;
			}
		}
		else
		{
			NewFolder->DisplayName = FolderPath;
			RootNodes.Add(NewFolder);
		}

		TSharedPtr<FLiveConfigPropertyTreeNode> NewFolderPtr = NewFolder;
		FolderMap.Add(FolderPath, NewFolderPtr);
		return NewFolderPtr;
	};

	for (const auto& PropDef : RawPropertyList)
	{
		bool bPassesTextFilter = FilterString.IsEmpty() || 
			PropDef->PropertyName.ToString().Contains(FilterString) || 
			PropDef->Description.Contains(FilterString);

		bool bPassesTagFilter = SelectedTag.IsNone() || PropDef->Tags.Contains(SelectedTag);

		if (bPassesTextFilter && bPassesTagFilter)
		{
			FString FullName = PropDef->PropertyName.ToString();
			int32 LastDot;
			if (FullName.FindLastChar('.', LastDot))
			{
				FString FolderPath = FullName.Left(LastDot);
				FString DisplayName = FullName.RightChop(LastDot + 1);

				TSharedPtr<FLiveConfigPropertyTreeNode> ParentFolder = GetOrCreateFolder(FolderPath);
				
				TSharedRef<FLiveConfigPropertyTreeNode> NewNode = MakeShared<FLiveConfigPropertyTreeNode>();
				NewNode->DisplayName = DisplayName;
				NewNode->FullPath = FullName;
				NewNode->PropertyDefinition = PropDef;
				
				if (ParentFolder.IsValid())
				{
					NewNode->Parent = ParentFolder;
					ParentFolder->Children.Add(NewNode);
				}
			}
			else
			{
				TSharedRef<FLiveConfigPropertyTreeNode> NewNode = MakeShared<FLiveConfigPropertyTreeNode>();
				NewNode->DisplayName = FullName;
				NewNode->FullPath = FullName;
				NewNode->PropertyDefinition = PropDef;
				RootNodes.Add(NewNode);
			}
		}
	}

	auto SortNodes = [](TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes)
	{
		Nodes.Sort([](const TSharedRef<FLiveConfigPropertyTreeNode>& A, const TSharedRef<FLiveConfigPropertyTreeNode>& B)
		{
			if (A->IsProperty() != B->IsProperty())
			{
				return !A->IsProperty(); // Folders (not property) first
			}
			return UE::ComparisonUtility::CompareNaturalOrder(A->DisplayName, B->DisplayName) < 0;
		});
	};

	SortNodes(RootNodes);
	for (auto& FolderPair : FolderMap)
	{
		SortNodes(FolderPair.Value->Children);
	}

	if (PropertyTreeView.IsValid())
	{
		PropertyTreeView->RequestTreeRefresh();
		
		// Restore expansion state or auto-expand all nodes when filtering
		for (auto& FolderPair : FolderMap)
		{
			bool bShouldExpand = !FilterString.IsEmpty() || ExpandedPaths.Contains(FolderPair.Key);
			if (bShouldExpand && FolderPair.Value.IsValid())
			{
				PropertyTreeView->SetItemExpansion(FolderPair.Value.ToSharedRef(), true);
			}
		}
	}
}

void SLiveConfigPropertyManager::UpdateAllTags()
{
	if (TagFilterBox.IsValid())
	{
		TagFilterBox->ClearChildren();

		auto CreateTagWidget = [this](FName TagName, FText DisplayName, bool bIsFilter) -> TSharedRef<SWidget>
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
	if (InTag.IsNone()) return RawPropertyList.Num();
	
	int32 Count = 0;
	for (const auto& PropDef : RawPropertyList)
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

void SLiveConfigPropertyManager::GetFlatVisibleProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutFlatList) const
{
	struct FLocal
	{
		static void Flatten(const TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes, TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutFlatList, TSharedPtr<STreeView<TSharedRef<FLiveConfigPropertyTreeNode>>> InTreeView)
		{
			for (auto& Node : Nodes)
			{
				OutFlatList.Add(Node);
				if (InTreeView.IsValid() && InTreeView->IsItemExpanded(Node))
				{
					Flatten(Node->Children, OutFlatList, InTreeView);
				}
			}
		}
	};
	FLocal::Flatten(RootNodes, OutFlatList, PropertyTreeView);
}

void SLiveConfigPropertyManager::NavigateToProperty(TSharedPtr<FLiveConfigPropertyTreeNode> CurrentItem, int32 Direction)
{
	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> FlatList;
	GetFlatVisibleProperties(FlatList);

	const int32 ItemCount = FlatList.Num();
	const int32 CurrentIndex = FlatList.IndexOfByPredicate([&CurrentItem](const TSharedRef<FLiveConfigPropertyTreeNode>& Node)
	{
		return Node == CurrentItem.ToSharedRef();
	});

	if (CurrentIndex != INDEX_NONE)
	{
		for (int32 i = CurrentIndex + Direction; i >= 0 && i < ItemCount; i += Direction)
		{
			if (FlatList[i]->IsProperty())
			{
				FlatList[i]->bNeedsFocus = true;
				if (PropertyTreeView.IsValid())
				{
					PropertyTreeView->RequestScrollIntoView(FlatList[i]);
					
					TSharedPtr<ITableRow> Row = PropertyTreeView->WidgetFromItem(FlatList[i]);
					if (Row.IsValid())
					{
						SLiveConfigPropertyRow* PropRow = static_cast<SLiveConfigPropertyRow*>(&Row->AsWidget().Get());
						if (PropRow)
						{
							PropRow->Tick(FGeometry(), 0, 0); // Force tick to apply focus
						}
					}
				}
				break;
			}
		}
	}
}

void SLiveConfigPropertyManager::SaveKnownTags()
{
	SaveKnownTags(KnownTags);
}

void SLiveConfigPropertyManager::GetMissingTags(TArray<FName>& OutMissingTags)
{
	ULiveConfigSystem* System = ULiveConfigSystem::Get();
	if (!System) return;

	TSet<FName> UsedTags;
	for (const auto& Pair : System->PropertyDefinitions)
	{
		for (const FName& PropertyTag : Pair.Value.Tags)
		{
			UsedTags.Add(PropertyTag);
		}
	}

	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();

	TSet<FName> KnownTagsSet(Settings->KnownTags);
	for (const FName& PropertyTag : UsedTags)
	{
		if (!KnownTagsSet.Contains(PropertyTag))
		{
			OutMissingTags.Add(PropertyTag);
		}
	}
}

void SLiveConfigPropertyManager::SaveKnownTags(const TArray<FName>& InKnownTags)
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	Settings->KnownTags = InKnownTags;
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
	UE_LOG(LogLiveConfig, Log, TEXT("Scrolling to property: %s"), *Property.ToString());

	if (SearchBox.IsValid())
	{
		SearchBox->SetText(FText::GetEmpty());
	}

	struct FLocal
	{
		static TSharedPtr<FLiveConfigPropertyTreeNode> FindNode(const TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes, const FString& FullPath)
		{
			for (const auto& Node : Nodes)
			{
				if (Node->FullPath == FullPath)
				{
					return Node;
				}
				if (FullPath.StartsWith(Node->FullPath + TEXT(".")))
				{
					TSharedPtr<FLiveConfigPropertyTreeNode> Found = FindNode(Node->Children, FullPath);
					if (Found.IsValid()) return Found;
				}
			}
			return nullptr;
		}
	};

	TSharedPtr<FLiveConfigPropertyTreeNode> TargetNode = FLocal::FindNode(RootNodes, Property.ToString());
	if (TargetNode.IsValid() && PropertyTreeView.IsValid())
	{
		// Expand all parents
		TSharedPtr<FLiveConfigPropertyTreeNode> Parent = TargetNode->Parent.Pin();
		while (Parent.IsValid())
		{
			PropertyTreeView->SetItemExpansion(Parent.ToSharedRef(), true);
			Parent = Parent->Parent.Pin();
		}

		PropertyTreeView->RequestScrollIntoView(TargetNode.ToSharedRef());
		PropertyTreeView->SetSelection(TargetNode.ToSharedRef());
		TargetNode->bNeedsFocus = true;
	}
}

void SLiveConfigPropertyManager::OnAddNewProperty()
{
	TSharedPtr<FLiveConfigPropertyDefinition> NewProp = MakeShared<FLiveConfigPropertyDefinition>();
	NewProp->PropertyName = FLiveConfigProperty(NAME_None);
	
	if (!SelectedTag.IsNone())
	{
		NewProp->Tags.Add(SelectedTag);
	}

	RawPropertyList.Add(NewProp);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
	
	ScrollToProperty(NewProp->PropertyName);
	UpdateAllTags();
}

void SLiveConfigPropertyManager::OnAddPropertyAtFolder(FString FolderPath)
{
	TSharedPtr<FLiveConfigPropertyDefinition> NewProp = MakeShared<FLiveConfigPropertyDefinition>();
	FString NewName = FolderPath + TEXT(".");
	NewProp->PropertyName = FLiveConfigProperty(FName(*NewName));
	
	if (!SelectedTag.IsNone())
	{
		NewProp->Tags.Add(SelectedTag);
	}

	RawPropertyList.Add(NewProp);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());

	ScrollToProperty(NewProp->PropertyName);
	UpdateAllTags();
}

void SLiveConfigPropertyManager::Save()
{
	ULiveConfigSystem* System = ULiveConfigSystem::Get();
	System->PropertyDefinitions.Empty();
	for (const auto& PropDef : RawPropertyList)
	{
		System->PropertyDefinitions.Add(PropDef->PropertyName, *PropDef);
	}
	System->SaveConfig();
	System->TryUpdateDefaultConfigFile();

	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		JsonSystem->SaveJsonToFiles();
	}

	// Notify system to refresh its base values
	if (ULiveConfigSystem* LiveConfigSystem = ULiveConfigSystem::Get())
	{
		LiveConfigSystem->RefreshFromSettings();
	}

	UpdateAllTags();
}

void SLiveConfigPropertyManager::OnPropertyRowChanged(TSharedPtr<FLiveConfigPropertyDefinition> OldDef, TSharedPtr<FLiveConfigPropertyDefinition> NewDef, ELiveConfigPropertyChangeType ChangeType)
{
	ULiveConfigSystem* LiveConfigSystem = ULiveConfigSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();

	auto IsValidPropertyName = [](const FLiveConfigProperty& Prop)
	{
		FString Str = Prop.ToString();
		return Prop.IsValid() && !Str.EndsWith(TEXT("."));
	};

	if (ChangeType == ELiveConfigPropertyChangeType::Name)
	{
		// If the name changed, we need to handle the old key in the settings
		if (OldDef.IsValid() && IsValidPropertyName(OldDef->PropertyName))
		{
			LiveConfigSystem->PropertyDefinitions.Remove(OldDef->PropertyName);
			if (JsonSystem)
			{
				JsonSystem->DeletePropertyFile(OldDef->PropertyName.GetName());
			}
		}
	}
	
	if (NewDef.IsValid() && IsValidPropertyName(NewDef->PropertyName))
	{
		LiveConfigSystem->PropertyDefinitions.Add(NewDef->PropertyName, *NewDef);
		if (JsonSystem)
		{
			JsonSystem->SavePropertyToFile(*NewDef);
		}
	}

	if (ChangeType == ELiveConfigPropertyChangeType::Name)
	{
		OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
	}

	LiveConfigSystem->RefreshFromSettings();

	UpdateAllTags();
	
	if (ChangeType == ELiveConfigPropertyChangeType::Name)
	{
		PendingScrollProperty = NewDef->PropertyName;
	}
}

void SLiveConfigPropertyManager::RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem)
{
	if (!InItem.IsValid())
	{
		return;
	}

	ULiveConfigSystem* System = ULiveConfigSystem::Get();
	System->PropertyDefinitions.Remove(InItem->PropertyName);

	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		JsonSystem->DeletePropertyFile(InItem->PropertyName.GetName());
	}

	RawPropertyList.Remove(InItem);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());

	System->RefreshFromSettings();

	UpdateAllTags();
}

void SLiveConfigPropertyManager::RemoveTag(FName TagName)
{
	if (TagName.IsNone() || TagName == LiveConfigTags::FromCurveTable)
	{
		return;
	}

	int32 Count = GetTagCount(TagName);
	if (Count > 0)
	{
		FText ConfirmMessage = FText::Format(
			LOCTEXT("DeleteTagInUseConfirm", "The tag '{0}' is in use by {1} property(s). Deleting it will remove it from all properties. Are you sure?"),
			FText::FromName(TagName),
			FText::AsNumber(Count)
		);

		if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmMessage, LOCTEXT("DeleteTagTitle", "Delete Tag")) != EAppReturnType::Yes)
		{
			return;
		}

		// Remove from properties
		ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();
		ULiveConfigSystem* System = ULiveConfigSystem::Get();

		for (const auto& PropDef : RawPropertyList)
		{
			if (PropDef->Tags.Remove(TagName) > 0)
			{
				System->PropertyDefinitions.Add(PropDef->PropertyName, *PropDef);
				if (JsonSystem)
				{
					JsonSystem->SavePropertyToFile(*PropDef);
				}
			}
		}
		System->SaveConfig();
		System->TryUpdateDefaultConfigFile();
	}

	KnownTags.Remove(TagName);
	if (SelectedTag == TagName)
	{
		SelectedTag = NAME_None;
	}

	// Persist KnownTags first
	SaveKnownTags();
	
	// Notify system to refresh its base values
	if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
	{
		System->RefreshFromSettings();
	}

	// Force a full refresh from the settings to ensure everything is in sync
	RefreshList();
}

bool SLiveConfigPropertyManager::IsNameDuplicate(FName Name) const
{
	int32 Count = 0;
	for (const auto& PropDef : RawPropertyList)
	{
		if (PropDef->PropertyName.GetName() == Name)
		{
			Count++;
		}
	}
	return Count > 1;
}

#undef LOCTEXT_NAMESPACE
