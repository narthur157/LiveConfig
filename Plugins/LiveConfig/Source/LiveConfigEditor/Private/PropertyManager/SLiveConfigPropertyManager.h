#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigSystem.h"

class SLiveConfigPropertyManager : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyManager, SCompoundWidget);
public:
	SLATE_BEGIN_ARGS(SLiveConfigPropertyManager)
	{}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);
	void RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem);
	void RemoveTag(FName TagName);
	void ScrollToProperty(FLiveConfigProperty Property);
	bool IsNameDuplicate(FName Name) const;

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FLiveConfigPropertyDefinition> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshList();
	void OnAddNewProperty();
	void OnAddNewTag();
	void OnFilterTextChanged(const FText& InFilterText);
	void UpdateAllTags();
	void OnTagFilterSelected(FName InTag);
	int32 GetTagCount(FName InTag) const;
	FSlateColor GetTagColor(FName InTag) const;
	void Save();
	void SaveKnownTags();
	void CheckForMissingTags();

	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> FullPropertyList;
	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> FilteredPropertyList;
	TArray<FName> AllTags;
	TArray<FName> KnownTags;
	FName SelectedTag;
	TSharedPtr<SListView<TSharedPtr<FLiveConfigPropertyDefinition>>> PropertyListView;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<class SVerticalBox> TagFilterBox;
	TSharedPtr<SWindow> NewTagWindow;
	bool bIsSaving = false;
};
