// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "LiveConfigSystem.h"

class ULiveConfigPropertyTreeSubsystem;

/**
 * Property tree nodes represent collapsing/expanding properties based on their paths
 * IE a property that is `MyProperty.SubProperty` has `MyProperty` as a folder
 */
struct FLiveConfigPropertyTreeNode : public TSharedFromThis<FLiveConfigPropertyTreeNode>
{
	FString DisplayName;
	FString FullPath;
	TSharedPtr<FLiveConfigPropertyDefinition> PropertyDefinition;
	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Children;
	TWeakPtr<FLiveConfigPropertyTreeNode> Parent;
	bool bIsExpanded = true;
	bool bNeedsFocus = false;

	bool IsProperty() const { return PropertyDefinition.IsValid() && PropertyDefinition->PropertyType != ELiveConfigPropertyType::Struct; }
	bool IsStruct() const { return PropertyDefinition.IsValid() && PropertyDefinition->PropertyType == ELiveConfigPropertyType::Struct; }
};

class SLiveConfigPropertyManager : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyManager, SCompoundWidget);
public:
	SLATE_BEGIN_ARGS(SLiveConfigPropertyManager)
	{}
	SLATE_END_ARGS();

	SLiveConfigPropertyManager();

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	void RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem);
	void RemoveTag(FName TagName);
	void ScrollToProperty(FLiveConfigProperty Property);
	bool IsNameDuplicate(FName Name) const;

	static void GetMissingTags(TArray<FName>& OutMissingTags);

private:
	static const TArray<TSharedPtr<FLiveConfigPropertyDefinition>>& GetPropertyDefinitions();

	TSharedRef<ITableRow> OnGenerateRow(TSharedRef<FLiveConfigPropertyTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedRef<FLiveConfigPropertyTreeNode> InItem, TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutChildren);
	void RefreshList();
	void OnAddNewProperty();
	void OnAddPropertyAtFolder(FString FolderPath);
	void OnPropertyRowMouseDown(TSharedRef<FLiveConfigPropertyTreeNode> SelectedItem);
	void OnAddNewTag(FName NewTag);
	void OnFilterTextChanged(const FText& InFilterText);
	void RefreshSearchFilter();
	void RefreshTags();
	void OnTagFilterSelected(FName InTag);
	int32 GetTagCount(FName InTag) const;
	void OnPropertyRowChanged(TSharedPtr<FLiveConfigPropertyDefinition> OldDef, TSharedPtr<FLiveConfigPropertyDefinition> NewDef, ELiveConfigPropertyChangeType ChangeType);
	void GetFlatVisibleProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutFlatList) const;
	void NavigateToProperty(TSharedPtr<FLiveConfigPropertyTreeNode> CurrentItem, int32 Direction);
	void SetAllNodesExpanded(bool bExpanded);
	void SetNodesExpandedRecursive(const TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& Nodes, bool bExpanded);
	void BulkAddTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName);
	void BulkRemoveTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName);
	void BulkDeleteProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes);
	TSharedPtr<SWidget> OnGetContextMenuContent();
	void OnTreeDoubleClick(TSharedRef<FLiveConfigPropertyTreeNode> LiveConfigPropertyTreeNode);
	void OnSelectionChanged(TSharedPtr<FLiveConfigPropertyTreeNode> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnManageRedirects();
	void OnCleanupUnusedProperties();

	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> RootNodes;
	FName SelectedTag;
	TSharedPtr<STreeView<TSharedRef<FLiveConfigPropertyTreeNode>>> PropertyTreeView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<class SVerticalBox> TagFilterBox;
	TSharedPtr<SWindow> NewTagWindow;
	bool bNeedsInitialRefresh = true;
	FLiveConfigProperty PendingScrollProperty;
};
