#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigSystem.h"

class SLiveConfigPropertyManager : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigPropertyManager)
	{}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);
	void RemoveProperty(TSharedPtr<FLiveConfigPropertyDefinition> InItem);
	void ScrollToProperty(FLiveConfigProperty Property);
	bool IsNameDuplicate(FName Name) const;

private:
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FLiveConfigPropertyDefinition> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshList();
	void OnAddNewProperty();
	void OnFilterTextChanged(const FText& InFilterText);
	void Save();

	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> FullPropertyList;
	TArray<TSharedPtr<FLiveConfigPropertyDefinition>> FilteredPropertyList;
	TSharedPtr<SListView<TSharedPtr<FLiveConfigPropertyDefinition>>> PropertyListView;
	TSharedPtr<class SSearchBox> SearchBox;
	bool bIsSaving = false;
};
