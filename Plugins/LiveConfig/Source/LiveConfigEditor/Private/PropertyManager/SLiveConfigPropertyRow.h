#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "LiveConfigSystem.h"

typedef SMultiColumnTableRow<TSharedPtr<FLiveConfigPropertyDefinition>> SPropertyRowParent;

class SLiveConfigPropertyRow : public SPropertyRowParent
{
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyRow, SPropertyRowParent);
public:
	SLiveConfigPropertyRow() : KnownTagsAttribute(*this) {}
	
	DECLARE_DELEGATE_OneParam(FOnDeleteProperty, TSharedPtr<FLiveConfigPropertyDefinition>);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FIsNameDuplicate, FName);

	SLATE_BEGIN_ARGS(SLiveConfigPropertyRow) {}
		SLATE_EVENT(FOnDeleteProperty, OnDeleteProperty);
		SLATE_EVENT(FIsNameDuplicate, IsNameDuplicate);
		SLATE_EVENT(FSimpleDelegate, OnChanged);
		SLATE_ARGUMENT(TFunction<FSlateColor(FName)>, GetTagColor);
		SLATE_ATTRIBUTE(TArray<FName>, KnownTags);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyDefinition> InItem, int32 InIndex);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	void RefreshTags();
	void OnTagChanged();

	TSharedPtr<FLiveConfigPropertyDefinition> Item;
	FOnDeleteProperty OnDeleteProperty;
	FIsNameDuplicate OnIsNameDuplicate;
	FSimpleDelegate OnChanged;
	TFunction<FSlateColor(FName)> GetTagColor;
	TSharedPtr<class SWrapBox> TagWrapBox;
	TSlateAttribute<TArray<FName>, EInvalidateWidgetReason::Layout> KnownTagsAttribute;
	bool bShowDescription = false;
};
