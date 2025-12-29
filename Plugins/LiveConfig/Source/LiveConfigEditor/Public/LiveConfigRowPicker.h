#pragma once

#include "CoreMinimal.h"
#include "LiveConfigRowCombo.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigSystem.h"

class SLiveConfigRowPicker : public SCompoundWidget
{
public:
    /** Callback when selection changes */
    DECLARE_DELEGATE_OneParam(FOnRowNameChanged, FLiveConfigRowName);
    
    SLATE_BEGIN_ARGS(SLiveConfigRowPicker)
        : _Filter()
        , _bReadOnly(false)
        , _bMultiSelect(false)
    {}
    SLATE_ARGUMENT(FName, Filter)
    SLATE_ARGUMENT(bool, bReadOnly)
    SLATE_ARGUMENT(bool, bMultiSelect)
        SLATE_EVENT(FOnRowNameChanged, OnRowNameChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Get the currently selected row name */
    FLiveConfigRowName GetSelectedRowName() const { return SelectedRowName; }

    /** Set the selected row name */
    void SetSelectedRowName(FLiveConfigRowName InRowName);

private:
    /** Refresh the list of available row names */
    void RefreshRowNameList();

    /** Filter the list based on search text */
    void OnFilterTextChanged(const FText& InFilterText);

    /** Handle selection in the list */
    void OnSelectionChanged(TSharedPtr<FLiveConfigRowName> InItem, ESelectInfo::Type SelectInfo);

    /** Handle adding a new row name */
    void OnAddNewRowName();

    /** Validate and add the new row name */
    void OnCommitNewRowName(const FText& InText, ETextCommit::Type CommitInfo);

    /** Get the display text for a row name */
    FText GetRowNameDisplayText(FLiveConfigRowName RowName) const;

    /** Get the description for a row name */
    FText GetRowNameDescription(FLiveConfigRowName RowName) const;

    /** Generate a row for the list view */
    TSharedRef<ITableRow> GenerateRow(TSharedPtr<FLiveConfigRowName> InItem, const TSharedRef<STableViewBase>& OwnerTable);

    FLiveConfigRowName SelectedRowName;
    FName Filter;
    bool bReadOnly = false;
    bool bMultiSelect = false;

    TArray<TSharedPtr<FLiveConfigRowName>> AvailableRowNames;
    TArray<TSharedPtr<FLiveConfigRowName>> FilteredRowNames;
    TSharedPtr<class SSearchBox> SearchBox;
    TSharedPtr<class SListView<TSharedPtr<FLiveConfigRowName>>> RowNameListView;
    TSharedPtr<class SEditableTextBox> AddNewTextBox;
    
    FOnRowNameChanged OnRowNameChanged;
};

