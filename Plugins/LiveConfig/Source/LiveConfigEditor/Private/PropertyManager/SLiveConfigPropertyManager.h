#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"
#include "LiveConfigSystem.h"

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
	SLATE_END_ARGS()

	SLiveConfigPropertyManager();

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	void RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem);
	void RemoveTag(FName TagName);
	void ScrollToProperty(FLiveConfigProperty Property);
	bool IsNameDuplicate(FName Name) const;

	static void SaveKnownTags(const TArray<FName>& InKnownTags);
	static void GetMissingTags(TArray<FName>& OutMissingTags);

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedRef<FLiveConfigPropertyTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedRef<FLiveConfigPropertyTreeNode> InItem, TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutChildren);
	void RefreshList();
	void OnAddNewProperty();
	void OnAddPropertyAtFolder(FString FolderPath);
	void OnAddNewTag();
	void OnFilterTextChanged(const FText& InFilterText);
	void UpdateAllTags();
	void OnTagFilterSelected(FName InTag);
	int32 GetTagCount(FName InTag) const;
	FSlateColor GetTagColor(FName InTag) const;
	void OnPropertyRowChanged(TSharedPtr<FLiveConfigPropertyDefinition> OldDef, TSharedPtr<FLiveConfigPropertyDefinition> NewDef, ELiveConfigPropertyChangeType ChangeType);
	void SaveKnownTags();
	void GetFlatVisibleProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>>& OutFlatList) const;
	void NavigateToProperty(TSharedPtr<FLiveConfigPropertyTreeNode> CurrentItem, int32 Direction);
	void OnContextMenuOpening(FMenuBuilder& MenuBuilder);
	void BulkAddTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName);
	void BulkRemoveTag(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes, FName TagName);
	void BulkDeleteProperties(TArray<TSharedRef<FLiveConfigPropertyTreeNode>> Nodes);
	TSharedPtr<SWidget> OnGetContextMenuContent();
	void OnSelectionChanged(TSharedPtr<FLiveConfigPropertyTreeNode> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnManageRedirects();
	void OnCleanupUnusedProperties();

	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> RawPropertyList;
	TArray<TSharedRef<FLiveConfigPropertyTreeNode>> RootNodes;
	TArray<FName> AllTags;
	TArray<FName> KnownTags;
	FName SelectedTag;
	TSharedPtr<STreeView<TSharedRef<FLiveConfigPropertyTreeNode>>> PropertyTreeView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<class SVerticalBox> TagFilterBox;
	TSharedPtr<SWindow> NewTagWindow;
	bool bNeedsInitialRefresh = true;
	FLiveConfigProperty PendingScrollProperty;
};
