#include "SLiveConfigPropertyManager.h"
#include "SLiveConfigPropertyRow.h"
#include "SLiveConfigTagRow.h"
#include "SLiveConfigTagPicker.h"
#include "SLiveConfigNewTagDialog.h"
#include "SLiveConfigNewPropertyDialog.h"
#include "SLiveConfigRedirectManager.h"
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
#include "LiveConfigEditorSettings.h"
#include "LiveConfigLib.h"
#include "SLiveConfigCleanupUnusedPropertiesWidget.h"
#include "Misc/MessageDialog.h"
#include "Misc/ComparisonUtility.h"

#define LOCTEXT_NAMESPACE "SLiveConfigPropertyManager"

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyManager);

SLiveConfigPropertyManager::SLiveConfigPropertyManager()
	: SelectedTag(NAME_None)
{
}

void SLiveConfigPropertyManager::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

void SLiveConfigPropertyManager::Construct(const FArguments& InArgs)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	{
		System.OnPropertiesUpdated.AddSP(this, &SLiveConfigPropertyManager::RefreshList);
		System.OnTagsChanged.AddSP(this, &SLiveConfigPropertyManager::RefreshTags);
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
								SLiveConfigNewTagDialog::OpenDialog(FOnTagCreated::CreateSP(this, &SLiveConfigPropertyManager::OnAddNewTag));
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
						.Text(LOCTEXT("AddProperty", "+ Add Property [A]"))
						.OnClicked_Lambda([this]()
						{
							OnAddNewProperty();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0, 0, 0)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.Text(LOCTEXT("CleanupUnusedProperties", "Cleanup Unused"))
						.ToolTipText(LOCTEXT("CleanupUnusedPropertiesTooltip", "Find and remove properties that are not used in any assets or config files"))
						.OnClicked_Lambda([this]()
						{
							OnCleanupUnusedProperties();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(5, 0, 0, 0)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.Text(LOCTEXT("ManageRedirects", "Manage Redirects"))
						.ToolTipText(LOCTEXT("ManageRedirectsTooltip", "View and clean up property redirects"))
						.OnClicked_Lambda([this]()
						{
							OnManageRedirects();
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
				.OnSelectionChanged(this, &SLiveConfigPropertyManager::OnSelectionChanged)
				.OnContextMenuOpening(this, &SLiveConfigPropertyManager::OnGetContextMenuContent)
				.SelectionMode(ESelectionMode::Multi)
				.HeaderRow(
					SNew(SHeaderRow)
					.Style(FAppStyle::Get(), "TableView.Header")
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Name)
					.DefaultLabel(LOCTEXT("NameColumn", "Name"))
					.FillWidth(0.35f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Type)
					.DefaultLabel(LOCTEXT("TypeColumn", "Type"))
					.FixedWidth(90.f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Value)
					.DefaultLabel(LOCTEXT("ValueColumn", "Value"))
					.FillWidth(0.2f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Tags)
					.DefaultLabel(LOCTEXT("TagsColumn", "Tags"))
					.FillWidth(0.3f)
					.HeaderContentPadding(FMargin(4.0f, 0.0f))
					+ SHeaderRow::Column(SLiveConfigPropertyRow::ColumnNames::Actions)
					.DefaultLabel(LOCTEXT("ActionsColumn", ""))
					.FixedWidth(180.0f)
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

FReply SLiveConfigPropertyManager::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetModifierKeys().IsControlDown() && InKeyEvent.GetKey() == EKeys::F)
	{
		if (SearchBox.IsValid())
		{
			FSlateApplication::Get().SetKeyboardFocus(SearchBox.ToSharedRef(), EFocusCause::SetDirectly);
			return FReply::Handled();
		}
	}
	else if (InKeyEvent.GetKey() == EKeys::A)
	{
		OnAddNewProperty();
		return FReply::Handled();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

TSharedRef<ITableRow> SLiveConfigPropertyManager::OnGenerateRow(TSharedRef<FLiveConfigPropertyTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLiveConfigPropertyRow, OwnerTable, InItem, 0)
		.OnDeleteProperty(this, &SLiveConfigPropertyManager::RemoveProperty)
		.OnAddPropertyAtFolder(this, &SLiveConfigPropertyManager::OnAddPropertyAtFolder)
		.OnBulkTagFolder_Lambda([this](FString FolderPath, FName TagName)
		{
			TArray<TSharedRef<FLiveConfigPropertyTreeNode>> FolderProperties;
			
			// Find the folder node
			auto FindFolder = [&FolderPath](auto& Self, const TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes) -> TSharedPtr<FLiveConfigPropertyTreeNode>
			{
				for (const auto& Node : Nodes)
				{
					if (Node->FullPath == FolderPath) return Node;
					TSharedPtr<FLiveConfigPropertyTreeNode> Found = Self(Self, Node->Children);
					if (Found.IsValid()) return Found;
				}
				return nullptr;
			};

			TSharedPtr<FLiveConfigPropertyTreeNode> FolderNode = FindFolder(FindFolder, RootNodes);
			if (FolderNode.IsValid())
			{
				// Collect all descendant properties
				auto CollectProperties = [&FolderProperties](auto& Self, TSharedRef<FLiveConfigPropertyTreeNode> Node) -> void
				{
					if (Node->IsProperty())
					{
						FolderProperties.Add(Node);
					}
					for (const auto& Child : Node->Children)
					{
						Self(Self, Child);
					}
				};
				
				for (const auto& Child : FolderNode->Children)
				{
					CollectProperties(CollectProperties, Child);
				}

				if (FolderProperties.Num() > 0)
				{
					BulkAddTag(FolderProperties, TagName);
				}
			}
		})
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
		.OnRequestScroll_Lambda([this](FLiveConfigProperty Property)
		{
			PendingScrollProperty = Property;
		});
}

void SLiveConfigPropertyManager::OnGetChildren(TSharedRef<FLiveConfigPropertyTreeNode> InItem, TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutChildren)
{
	OutChildren.Append(InItem->Children);
}

void SLiveConfigPropertyManager::RefreshList()
{
	RawPropertyList.Empty();
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	for (auto& Pair : System.PropertyDefinitions)
	{
		RawPropertyList.Add(MakeShared<FLiveConfigPropertyDefinition>(Pair.Value));
	}

	RawPropertyList.Sort([](const TSharedPtr<FLiveConfigPropertyDefinition>& A, const TSharedPtr<FLiveConfigPropertyDefinition>& B)
	{
		return UE::ComparisonUtility::CompareNaturalOrder(A->PropertyName.ToString(), B->PropertyName.ToString()) < 0;
	});
	
	RefreshTags();
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

		// Check if we already have a property that is a struct with this name
		for (const auto& PropDef : RawPropertyList)
		{
			if (PropDef->PropertyName.ToString() == FolderPath && PropDef->PropertyType == ELiveConfigPropertyType::Struct)
			{
				// This struct property will be created later in the loop and will replace the folder if it's already here.
				// But we need a node NOW to attach children to.
				TSharedRef<FLiveConfigPropertyTreeNode> NewNode = MakeShared<FLiveConfigPropertyTreeNode>();
				NewNode->FullPath = FolderPath;
				NewNode->PropertyDefinition = PropDef;
				
				int32 LastDot;
				if (FolderPath.FindLastChar('.', LastDot))
				{
					NewNode->DisplayName = FolderPath.RightChop(LastDot + 1);
					FString ParentPath = FolderPath.Left(LastDot);
					TSharedPtr<FLiveConfigPropertyTreeNode> ParentFolder = GetOrCreateFolder(ParentPath);
					if (ParentFolder.IsValid())
					{
						ParentFolder->Children.Add(NewNode);
						NewNode->Parent = ParentFolder;
					}
				}
				else
				{
					NewNode->DisplayName = FolderPath;
					RootNodes.Add(NewNode);
				}

				TSharedPtr<FLiveConfigPropertyTreeNode> NewNodePtr = NewNode;
				FolderMap.Add(FolderPath, NewNodePtr);
				return NewNodePtr;
			}
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
			
			// If it's a struct property and it already exists in FolderMap (created by GetOrCreateFolder)
			// we skip it here as it was already added.
			if (PropDef->PropertyType == ELiveConfigPropertyType::Struct && FolderMap.Contains(FullName))
			{
				continue;
			}

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
				
				if (PropDef->PropertyType == ELiveConfigPropertyType::Struct)
				{
					// If it's a struct and we already have a folder with the same name,
					// we should probably merge them or ensure the struct node becomes the parent.
					if (TSharedPtr<FLiveConfigPropertyTreeNode>* ExistingFolder = FolderMap.Find(FullName))
					{
						// Merge existing folder into this struct node
						NewNode->Children = (*ExistingFolder)->Children;
						for (auto& Child : NewNode->Children)
						{
							Child->Parent = NewNode;
						}
						
						// Replace folder in RootNodes or its parent's children
						if (TSharedPtr<FLiveConfigPropertyTreeNode> FolderParent = (*ExistingFolder)->Parent.Pin())
						{
							FolderParent->Children.Remove((*ExistingFolder).ToSharedRef());
							FolderParent->Children.Add(NewNode);
							NewNode->Parent = FolderParent;
						}
						else
						{
							RootNodes.Remove((*ExistingFolder).ToSharedRef());
							RootNodes.Add(NewNode);
						}
						
						FolderMap[FullName] = NewNode;
					}
					else
					{
						RootNodes.Add(NewNode);
						FolderMap.Add(FullName, NewNode);
					}
				}
				else
				{
					RootNodes.Add(NewNode);
				}
			}
		}
	}

	auto SortNodes = [](TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes)
	{
		Nodes.Sort([](const TSharedRef<FLiveConfigPropertyTreeNode>& A, const TSharedRef<FLiveConfigPropertyTreeNode>& B)
		{
			bool bAIsFolder = !A->PropertyDefinition.IsValid();
			bool bAIsStruct = A->IsStruct();
			bool bBIsFolder = !B->PropertyDefinition.IsValid();
			bool bBIsStruct = B->IsStruct();

			bool bAIsParent = bAIsFolder || bAIsStruct;
			bool bBIsParent = bBIsFolder || bBIsStruct;

			if (bAIsParent != bBIsParent)
			{
				return bAIsParent; // Parents (Folders/Structs) first
			}
			
			if (A->DisplayName.IsEmpty() || B->DisplayName.IsEmpty())
			{
				return false;
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

void SLiveConfigPropertyManager::RefreshTags()
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
								return TagName.IsNone() ? FLinearColor::Transparent : ULiveConfigLib::GetTagColor(TagName).GetSpecifiedColor();
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

		for (const FName& PropertyTag : ULiveConfigSystem::Get().PropertyTags)
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

void SLiveConfigPropertyManager::OnAddNewTag(FName NewTag)
{
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

TSharedPtr<SWidget> SLiveConfigPropertyManager::OnGetContextMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> SelectedItems = PropertyTreeView->GetSelectedItems();
	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> FlatVisibleItems;
	GetFlatVisibleProperties(FlatVisibleItems);

	auto BuildAddTagSubMenu = [this](FMenuBuilder& SubMenuBuilder, TArray<TSharedRef<FLiveConfigPropertyTreeNode>> TargetNodes)
	{
		// Filter out non-property nodes from target
		TArray<TSharedRef<FLiveConfigPropertyTreeNode>> PropertyNodes;
		for (auto& Node : TargetNodes)
		{
			if (Node->IsProperty())
			{
				PropertyNodes.Add(Node);
			}
		}

		if (PropertyNodes.Num() == 0)
		{
			SubMenuBuilder.AddMenuEntry(
				LOCTEXT("NoPropertiesSelected", "No properties in selection."),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([](){ return false; }))
			);
			return;
		}

		SubMenuBuilder.AddWidget(
			SNew(SLiveConfigTagPicker)
			.TagOptions(ULiveConfigSystem::Get().PropertyTags)
			.OnTagSelected_Lambda([this, PropertyNodes](FName SelectedTag)
			{
				BulkAddTag(PropertyNodes, SelectedTag);
			}),
			FText::GetEmpty(),
			true // No padding
		);
	};

	auto BuildRemoveTagSubMenu = [this](FMenuBuilder& SubMenuBuilder, TArray<TSharedRef<FLiveConfigPropertyTreeNode>> TargetNodes)
	{
		// Filter out non-property nodes from target
		TArray<TSharedRef<FLiveConfigPropertyTreeNode>> PropertyNodes;
		for (auto& Node : TargetNodes)
		{
			if (Node->IsProperty())
			{
				PropertyNodes.Add(Node);
			}
		}

		if (PropertyNodes.Num() == 0)
		{
			return;
		}

		TArray<FName> CommonTags;
		for (const auto& Node : PropertyNodes)
		{
			for (const FName& Tag : Node->PropertyDefinition->Tags)
			{
				CommonTags.AddUnique(Tag);
			}
		}

		if (CommonTags.Num() == 0)
		{
			SubMenuBuilder.AddMenuEntry(
				LOCTEXT("NoTagsToRemove", "No tags to remove."),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([](){ return false; }))
			);
			return;
		}

		CommonTags.Sort([](const FName& A, const FName& B) { return A.Compare(B) < 0; });

		for (const FName& Tag : CommonTags)
		{
			SubMenuBuilder.AddMenuEntry(
				FText::FromName(Tag),
				FText::Format(LOCTEXT("RemoveTagTooltip", "Remove tag '{0}' from properties"), FText::FromName(Tag)),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, PropertyNodes, Tag]()
				{
					BulkRemoveTag(PropertyNodes, Tag);
				}))
			);
		}
	};

	if (SelectedItems.Num() > 0)
	{
		TArray<TSharedRef<FLiveConfigPropertyTreeNode>> SelectedPropertyNodes;
		for (auto& Node : SelectedItems)
		{
			if (Node->IsProperty())
			{
				SelectedPropertyNodes.Add(Node);
			}
		}

		if (SelectedPropertyNodes.Num() > 0)
		{
			MenuBuilder.BeginSection("SelectedProperties", LOCTEXT("SelectedPropertiesSection", "Selected Properties"));
			
			MenuBuilder.AddSubMenu(
				FText::Format(LOCTEXT("TagSelectedProperties", "Add Tag to Selected ({0})"), FText::AsNumber(SelectedPropertyNodes.Num())),
				LOCTEXT("TagSelectedPropertiesTooltip", "Add a tag to all currently selected properties"),
				FNewMenuDelegate::CreateLambda([BuildAddTagSubMenu, SelectedPropertyNodes](FMenuBuilder& SubMenuBuilder)
				{
					BuildAddTagSubMenu(SubMenuBuilder, SelectedPropertyNodes);
				})
			);

			MenuBuilder.AddSubMenu(
				FText::Format(LOCTEXT("RemoveTagSelectedProperties", "Remove Tag from Selected ({0})"), FText::AsNumber(SelectedPropertyNodes.Num())),
				LOCTEXT("RemoveTagSelectedPropertiesTooltip", "Remove a tag from all currently selected properties"),
				FNewMenuDelegate::CreateLambda([BuildRemoveTagSubMenu, SelectedPropertyNodes](FMenuBuilder& SubMenuBuilder)
				{
					BuildRemoveTagSubMenu(SubMenuBuilder, SelectedPropertyNodes);
				})
			);

			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("DeleteSelectedProperties", "Delete Selected Properties ({0})"), FText::AsNumber(SelectedPropertyNodes.Num())),
				LOCTEXT("DeleteSelectedPropertiesTooltip", "Delete all currently selected properties"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
				FUIAction(FExecuteAction::CreateLambda([this, SelectedPropertyNodes]()
				{
					BulkDeleteProperties(SelectedPropertyNodes);
				}))
			);

			MenuBuilder.EndSection();
		}
	}

	if (FlatVisibleItems.Num() > 0)
	{
		TArray<TSharedRef<FLiveConfigPropertyTreeNode>> VisiblePropertyNodes;
		for (auto& Node : FlatVisibleItems)
		{
			if (Node->IsProperty())
			{
				VisiblePropertyNodes.Add(Node);
			}
		}

		if (VisiblePropertyNodes.Num() > 0)
		{
			MenuBuilder.BeginSection("VisibleProperties", LOCTEXT("VisiblePropertiesSection", "Visible Properties"));

			MenuBuilder.AddSubMenu(
				FText::Format(LOCTEXT("TagAllVisibleProperties", "Add Tag to All Visible ({0})"), FText::AsNumber(VisiblePropertyNodes.Num())),
				LOCTEXT("TagAllVisiblePropertiesTooltip", "Add a tag to all properties in the current filtered list"),
				FNewMenuDelegate::CreateLambda([BuildAddTagSubMenu, VisiblePropertyNodes](FMenuBuilder& SubMenuBuilder)
				{
					BuildAddTagSubMenu(SubMenuBuilder, VisiblePropertyNodes);
				})
			);

			MenuBuilder.AddSubMenu(
				FText::Format(LOCTEXT("RemoveTagAllVisibleProperties", "Remove Tag from All Visible ({0})"), FText::AsNumber(VisiblePropertyNodes.Num())),
				LOCTEXT("RemoveTagAllVisiblePropertiesTooltip", "Remove a tag from all properties in the current filtered list"),
				FNewMenuDelegate::CreateLambda([BuildRemoveTagSubMenu, VisiblePropertyNodes](FMenuBuilder& SubMenuBuilder)
				{
					BuildRemoveTagSubMenu(SubMenuBuilder, VisiblePropertyNodes);
				})
			);

			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("DeleteAllVisibleProperties", "Delete All Visible Properties ({0})"), FText::AsNumber(VisiblePropertyNodes.Num())),
				LOCTEXT("DeleteAllVisiblePropertiesTooltip", "Delete all properties in the current filtered list"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
				FUIAction(FExecuteAction::CreateLambda([this, VisiblePropertyNodes]()
				{
					BulkDeleteProperties(VisiblePropertyNodes);
				}))
			);

			MenuBuilder.EndSection();
		}
	}

	return MenuBuilder.MakeWidget();
}

void SLiveConfigPropertyManager::BulkAddTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName)
{
	bool bChanged = false;
	for (auto& Node : Nodes)
	{
		if (Node->IsProperty())
		{
			if (!Node->PropertyDefinition->Tags.Contains(TagName))
			{
				TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Node->PropertyDefinition);
				Node->PropertyDefinition->Tags.Add(TagName);
				OnPropertyRowChanged(OldDef, Node->PropertyDefinition, ELiveConfigPropertyChangeType::Tags);
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		PropertyTreeView->RequestTreeRefresh();
	}
}

void SLiveConfigPropertyManager::BulkRemoveTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName)
{
	bool bChanged = false;
	for (auto& Node : Nodes)
	{
		if (Node->IsProperty())
		{
			if (Node->PropertyDefinition->Tags.Contains(TagName))
			{
				TSharedPtr<FLiveConfigPropertyDefinition> OldDef = MakeShared<FLiveConfigPropertyDefinition>(*Node->PropertyDefinition);
				Node->PropertyDefinition->Tags.Remove(TagName);
				OnPropertyRowChanged(OldDef, Node->PropertyDefinition, ELiveConfigPropertyChangeType::Tags);
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		PropertyTreeView->RequestTreeRefresh();
	}
}

void SLiveConfigPropertyManager::BulkDeleteProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes)
{
	if (Nodes.Num() == 0)
	{
		return;
	}

	FText ConfirmMessage = FText::Format(
		LOCTEXT("BulkDeleteConfirm", "Are you sure you want to delete {0} selected property(s)?"),
		FText::AsNumber(Nodes.Num())
	);

	if (FMessageDialog::Open(EAppMsgType::YesNo, ConfirmMessage, LOCTEXT("BulkDeleteTitle", "Delete Properties")) != EAppReturnType::Yes)
	{
		return;
	}

	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();

	for (auto& Node : Nodes)
	{
		if (Node->IsProperty() && Node->PropertyDefinition.IsValid())
		{
			System.PropertyDefinitions.Remove(Node->PropertyDefinition->PropertyName);

			if (JsonSystem)
			{
				JsonSystem->DeletePropertyFile(Node->PropertyDefinition->PropertyName.GetName());
			}

			RawPropertyList.Remove(Node->PropertyDefinition);
		}
	}

	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());

	System.RebuildConfigCache();
	RefreshTags();
}

void SLiveConfigPropertyManager::OnSelectionChanged(TSharedPtr<FLiveConfigPropertyTreeNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo == ESelectInfo::Direct || !SelectedItem.IsValid())
	{
		return;
	}

	// If a folder (struct with no definition, or explicit struct type) is selected, select all its children
	if (SelectedItem->IsStruct() || (!SelectedItem->PropertyDefinition.IsValid() && SelectedItem->Children.Num() > 0))
	{
		TArray<TSharedRef<FLiveConfigPropertyTreeNode>> ToSelect;
			
		TFunction<void(TSharedRef<FLiveConfigPropertyTreeNode>)> CollectChildren = [&](TSharedRef<FLiveConfigPropertyTreeNode> Node)
		{
			for (auto& Child : Node->Children)
			{
				ToSelect.Add(Child);
				CollectChildren(Child);
			}
		};

		CollectChildren(SelectedItem.ToSharedRef());

		if (ToSelect.Num() > 0)
		{
			PropertyTreeView->SetItemSelection(ToSelect, true, ESelectInfo::Direct);
		}
	}
}

void SLiveConfigPropertyManager::GetMissingTags(TArray<FName>& OutMissingTags)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	TSet<FName> UsedTags;
	for (const auto& Pair : System.PropertyDefinitions)
	{
		for (const FName& PropertyTag : Pair.Value.Tags)
		{
			UsedTags.Add(PropertyTag);
		}
	}

	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();

	TSet<FName> PropertyTagsSet(ULiveConfigSystem::Get().PropertyTags);
	for (const FName& PropertyTag : UsedTags)
	{
		if (!PropertyTagsSet.Contains(PropertyTag))
		{
			OutMissingTags.Add(PropertyTag);
		}
	}
}

void SLiveConfigPropertyManager::OnTagFilterSelected(FName InTag)
{
	SelectedTag = InTag;
	RefreshTags(); // Refresh colors/highlights
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
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
	ELiveConfigPropertyType InitialType = ELiveConfigPropertyType::String;
	SLiveConfigNewPropertyDialog::OpenDialog(TEXT(""), InitialType, FOnPropertyCreated::CreateLambda([this](const FLiveConfigPropertyDefinition& NewDef)
	{
		TSharedPtr<FLiveConfigPropertyDefinition> NewProp = MakeShared<FLiveConfigPropertyDefinition>(NewDef);
		
		ULiveConfigSystem::Get().SaveProperty(*NewProp);
		RawPropertyList.Add(NewProp);
		
		OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
		
		// Ensure it's scrolled to and selected
		ScrollToProperty(NewProp->PropertyName);
		
		RefreshTags();
		
		ULiveConfigSystem::Get().RebuildConfigCache();
	}));
}

void SLiveConfigPropertyManager::OnAddPropertyAtFolder(FString FolderPath)
{
	FString NewName = FolderPath + TEXT(".");
	ELiveConfigPropertyType InitialType = ELiveConfigPropertyType::String;
	
	SLiveConfigNewPropertyDialog::OpenDialog(NewName, InitialType, FOnPropertyCreated::CreateLambda([this](const FLiveConfigPropertyDefinition& NewDef)
	{
		TSharedPtr<FLiveConfigPropertyDefinition> NewProp = MakeShared<FLiveConfigPropertyDefinition>(NewDef);
		
		ULiveConfigSystem::Get().SaveProperty(*NewProp);
		RawPropertyList.Add(NewProp);
		
		OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
		ScrollToProperty(NewProp->PropertyName);
		RefreshTags();
	}));
}

void SLiveConfigPropertyManager::OnPropertyRowChanged(TSharedPtr<FLiveConfigPropertyDefinition> OldDef, TSharedPtr<FLiveConfigPropertyDefinition> NewDef, ELiveConfigPropertyChangeType ChangeType)
{
	ULiveConfigSystem& LiveConfigSystem = ULiveConfigSystem::Get();

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
			// Check the redirect mode setting
			const ULiveConfigEditorSettings* EditorSettings = GetDefault<ULiveConfigEditorSettings>();
			bool bCreateRedirector = false;

			switch (EditorSettings->RedirectMode)
			{
			case ELiveConfigRedirectMode::AlwaysCreate:
				bCreateRedirector = true;
				break;

			case ELiveConfigRedirectMode::NeverCreate:
				bCreateRedirector = false;
				break;

			case ELiveConfigRedirectMode::Prompt:
				{
					const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo,
						LOCTEXT("CreateRedirectorQuestion", "Would you like to create a property redirector for this rename? This will ensure existing references continue to work."),
						LOCTEXT("CreateRedirectorTitle", "Create Redirector?"));
					bCreateRedirector = (Result == EAppReturnType::Yes);
				}
				break;
			}

			LiveConfigSystem.RenameProperty(OldDef->PropertyName, NewDef->PropertyName, bCreateRedirector);

			// Update all our local TSharedPtrs if they were part of a struct rename
			if (OldDef->PropertyType == ELiveConfigPropertyType::Struct)
			{
				FString OldPrefix = OldDef->PropertyName.ToString() + TEXT(".");
				FString NewPrefix = NewDef->PropertyName.ToString() + TEXT(".");

				for (const auto& PropDef : RawPropertyList)
				{
					if (PropDef->PropertyName.ToString().StartsWith(OldPrefix))
					{
						FString RelativeName = PropDef->PropertyName.ToString().RightChop(OldPrefix.Len());
						PropDef->PropertyName = FLiveConfigProperty(NewPrefix + RelativeName);
					}
				}
			}
		}
	}
	else if (NewDef.IsValid() && IsValidPropertyName(NewDef->PropertyName))
	{
		ULiveConfigSystem::Get().SaveProperty(*NewDef);
	}

	if (ChangeType == ELiveConfigPropertyChangeType::Name)
	{
		OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
	}

	LiveConfigSystem.RebuildConfigCache();

	RefreshTags();
	
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

	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	System.PropertyDefinitions.Remove(InItem->PropertyName);

	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		JsonSystem->DeletePropertyFile(InItem->PropertyName.GetName());
	}

	RawPropertyList.Remove(InItem);
	OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());

	System.RebuildConfigCache();

	RefreshTags();
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

		for (const auto& PropDef : RawPropertyList)
		{
			if (PropDef->Tags.Remove(TagName) > 0)
			{
				ULiveConfigSystem::Get().SaveProperty(*PropDef);
			}
		}
	}

	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	System.PropertyTags.Remove(TagName);
	System.TryUpdateDefaultConfigFile();
	
	if (SelectedTag == TagName)
	{
		SelectedTag = NAME_None;
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

void SLiveConfigPropertyManager::OnManageRedirects()
{
	TSharedPtr<SWindow> RedirectWindow = SNew(SWindow)
		.Title(LOCTEXT("ManageRedirectsWindowTitle", "Manage Property Redirects"))
		.ClientSize(FVector2D(900, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SLiveConfigRedirectManager)
		];

	FSlateApplication::Get().AddWindow(RedirectWindow.ToSharedRef());
}

void SLiveConfigPropertyManager::OnCleanupUnusedProperties()
{
	TSharedPtr<SWindow> CleanupWindow = SNew(SWindow)
		.Title(LOCTEXT("CleanupUnusedPropertiesWindowTitle", "Cleanup Unused Live Config Properties"))
		.ClientSize(FVector2D(600, 500))
		.SupportsMaximize(true)
		.SupportsMinimize(false)
		[
			SNew(SLiveConfigCleanupUnusedPropertiesWidget)
		];

	FSlateApplication::Get().AddWindow(CleanupWindow.ToSharedRef());

	CleanupWindow->GetOnWindowClosedEvent().AddLambda([this](const TSharedRef<SWindow>&)
	{
		RefreshList();
	});
}

#undef LOCTEXT_NAMESPACE
