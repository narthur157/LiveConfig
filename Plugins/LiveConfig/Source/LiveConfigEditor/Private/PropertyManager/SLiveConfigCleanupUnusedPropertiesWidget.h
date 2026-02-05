#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FUnusedLiveConfigPropertyItem : public TSharedFromThis<FUnusedLiveConfigPropertyItem>
{
public:
	FUnusedLiveConfigPropertyItem(FName InPropertyName)
		: PropertyName(InPropertyName)
		, CheckState(ECheckBoxState::Checked)
	{}

	FName PropertyName;
	ECheckBoxState CheckState;
};

class SLiveConfigCleanupUnusedPropertiesWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigCleanupUnusedPropertiesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> MakeUnusedPropertyListItemWidget(TSharedPtr<FUnusedLiveConfigPropertyItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	
	void PopulateUnusedProperties();
	FReply OnRemovePressed();
	bool IsRemoveEnabled() const;

	ECheckBoxState GetToggleSelectedState() const;
	void OnToggleSelectedCheckBox(ECheckBoxState InNewState);

	FText GetDescriptionText() const;

	TArray<TSharedPtr<FUnusedLiveConfigPropertyItem>> UnusedProperties;
	TSharedPtr<SListView<TSharedPtr<FUnusedLiveConfigPropertyItem>>> UnusedPropertiesListView;
};
