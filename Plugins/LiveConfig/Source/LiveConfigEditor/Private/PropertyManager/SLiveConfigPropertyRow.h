#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "SLiveConfigPropertyManager.h"

typedef SMultiColumnTableRow<TSharedRef<FLiveConfigPropertyTreeNode>> SPropertyRowParent;

class SLiveConfigPropertyRow : public SPropertyRowParent
{
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyRow, SPropertyRowParent);
public:
	struct ColumnNames
	{
		static const FName Name;
		static const FName Description;
		static const FName Type;
		static const FName Value;
		static const FName Tags;
		static const FName Actions;
	};

	SLiveConfigPropertyRow() : KnownTagsAttribute(*this) {}
	
	DECLARE_DELEGATE_ThreeParams(FOnPropertyPropertyChanged, TSharedPtr<FLiveConfigPropertyDefinition>, TSharedPtr<FLiveConfigPropertyDefinition>, ELiveConfigPropertyChangeType);
	DECLARE_DELEGATE_OneParam(FOnDeleteProperty, TSharedPtr<FLiveConfigPropertyDefinition>);
	DECLARE_DELEGATE_OneParam(FOnAddPropertyAtFolder, FString);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FIsNameDuplicate, FName);
	DECLARE_DELEGATE(FOnRequestRefresh);

	SLATE_BEGIN_ARGS(SLiveConfigPropertyRow) {}
		SLATE_EVENT(FOnDeleteProperty, OnDeleteProperty);
		SLATE_EVENT(FOnAddPropertyAtFolder, OnAddPropertyAtFolder);
		SLATE_EVENT(FIsNameDuplicate, IsNameDuplicate);
		SLATE_EVENT(FOnPropertyPropertyChanged, OnChanged);
		SLATE_EVENT(FOnRequestRefresh, OnRequestRefresh);
		SLATE_ARGUMENT(TFunction<FSlateColor(FName)>, GetTagColor);
		SLATE_ATTRIBUTE(TArray<FName>, KnownTags);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyTreeNode>
	               InItem, int32 InIndex);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

	static constexpr float RowHeight = 32.0f;

private:
	TSharedRef<SWidget> GenerateNameColumnWidget();
	TSharedRef<SWidget> GenerateDescriptionColumnWidget();
	TSharedRef<SWidget> GenerateTypeColumnWidget();
	TSharedRef<SWidget> GenerateValueColumnWidget();
	TSharedRef<SWidget> GenerateTagsColumnWidget();
	TSharedRef<SWidget> GenerateActionsColumnWidget();

	void RefreshTags();
	void OnTagChanged();

	TSharedPtr<FLiveConfigPropertyTreeNode> Item;
	FOnDeleteProperty OnDeleteProperty;
	FOnAddPropertyAtFolder OnAddPropertyAtFolder;
	FIsNameDuplicate OnIsNameDuplicate;
	FOnPropertyPropertyChanged OnChanged;
	FOnRequestRefresh OnRequestRefresh;
	TFunction<FSlateColor(FName)> GetTagColor;
	TSharedPtr<class SWrapBox> TagWrapBox;
	TSharedPtr<class SEditableTextBox> NameTextBox;
	TSlateAttribute<TArray<FName>, EInvalidateWidgetReason::Layout> KnownTagsAttribute;
	bool bNeedsFocus = false;
};
