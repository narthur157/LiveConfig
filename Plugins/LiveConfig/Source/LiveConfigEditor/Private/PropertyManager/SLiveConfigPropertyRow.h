#pragma once

#include "CoreMinimal.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "SLiveConfigPropertyManager.h"

typedef SMultiColumnTableRow<TSharedRef<FLiveConfigPropertyTreeNode>> SPropertyRowParent;

class SLiveConfigPropertyRow : public SPropertyRowParent
{
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

	SLiveConfigPropertyRow();
	
	DECLARE_DELEGATE_ThreeParams(FOnPropertyPropertyChanged, TSharedPtr<FLiveConfigPropertyDefinition>, TSharedPtr<FLiveConfigPropertyDefinition>, ELiveConfigPropertyChangeType);
	DECLARE_DELEGATE_OneParam(FOnDeleteProperty, TSharedPtr<FLiveConfigPropertyDefinition>);
	DECLARE_DELEGATE_OneParam(FOnAddPropertyAtFolder, FString);
	DECLARE_DELEGATE_TwoParams(FOnBulkTagFolder, FString, FName);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FIsNameDuplicate, FName);
	DECLARE_DELEGATE(FOnRequestRefresh);
	DECLARE_DELEGATE_OneParam(FOnNavigatePropertyName, TSharedPtr<FLiveConfigPropertyTreeNode>);
	DECLARE_DELEGATE_OneParam(FOnNavigateValue, TSharedPtr<FLiveConfigPropertyTreeNode>);

	SLATE_BEGIN_ARGS(SLiveConfigPropertyRow) {}
		SLATE_EVENT(FOnDeleteProperty, OnDeleteProperty)
		SLATE_EVENT(FOnAddPropertyAtFolder, OnAddPropertyAtFolder)
		SLATE_EVENT(FOnBulkTagFolder, OnBulkTagFolder)
		SLATE_EVENT(FIsNameDuplicate, IsNameDuplicate)
		SLATE_EVENT(FOnPropertyPropertyChanged, OnChanged)
		SLATE_EVENT(FOnRequestRefresh, OnRequestRefresh)
		SLATE_EVENT(FOnNavigatePropertyName, OnNavigateDown)
		SLATE_EVENT(FOnNavigatePropertyName, OnNavigateUp)
		SLATE_EVENT(FOnNavigateValue, OnNavigateValue)
		SLATE_EVENT(FSimpleDelegate, OnAddNewTag)
		SLATE_ARGUMENT(TFunction<FSlateColor(FName)>, GetTagColor)
		SLATE_ATTRIBUTE(TArray<FName>, KnownTags)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTable, TSharedPtr<FLiveConfigPropertyTreeNode>
	               InItem, int32 InIndex);

	void RequestValueFocus() { bNeedsValueFocus = true; }

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

	TSharedRef<SWidget> OnGetStructPickerMenu();
	void OnStructPicked(const UScriptStruct* ChosenStruct);
	void GenerateSubPropertiesForStruct(const UScriptStruct* Struct);

	void RefreshTags();
	void OnTagChanged();
	void OnAddNewTagClicked();
	bool VerifyValueText(const FText& NewText, FText& OutError);
	void ValueTextCommitted(const FText& NewText, ETextCommit::Type CommitType);

	TSharedPtr<FLiveConfigPropertyTreeNode> Item;
	FOnDeleteProperty OnDeleteProperty;
	FOnAddPropertyAtFolder OnAddPropertyAtFolder;
	FOnBulkTagFolder OnBulkTagFolder;
	FIsNameDuplicate OnIsNameDuplicate;
	FOnPropertyPropertyChanged OnChanged;
	FOnRequestRefresh OnRequestRefresh;
	FOnNavigatePropertyName OnNavigateDown;
	FOnNavigatePropertyName OnNavigateUp;
	FOnNavigateValue OnNavigateValue;
	TFunction<FSlateColor(FName)> GetTagColor;
	TSharedPtr<class SWrapBox> TagWrapBox;
	TSharedPtr<class SEditableTextBox> NameTextBox;
	TSharedPtr<class SEditableTextBox> ValueTextBox;
	TSharedPtr<class SCheckBox> ValueCheckBox;
	TSlateAttribute<TArray<FName>, EInvalidateWidgetReason::Layout> KnownTagsAttribute;
	bool bNeedsFocus = false;
	bool bNeedsValueFocus = false;
	bool bIsCommitting = false;
	bool bJustFinishedEnterCommit = false;
	FSimpleDelegate OnAddNewTag;
};
