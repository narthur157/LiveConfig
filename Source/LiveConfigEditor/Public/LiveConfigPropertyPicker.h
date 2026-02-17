// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigPropertyCombo.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "LiveConfigSystem.h"

class SLiveConfigPropertyPicker : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigPropertyPicker, SCompoundWidget);
public:
    /** Callback when selection changes */
    DECLARE_DELEGATE_OneParam(FOnPropertySelected, FLiveConfigProperty);
    
    SLATE_BEGIN_ARGS(SLiveConfigPropertyPicker)
        : _Filter()
        , _FilterType()
        , _StructFilter(nullptr)
        , _bReadOnly(false)
        , _bMultiSelect(false)
    {}
        SLATE_ARGUMENT(FName, Filter);
        SLATE_ARGUMENT(TOptional<ELiveConfigPropertyType>, FilterType);
        SLATE_ARGUMENT(UScriptStruct*, StructFilter);
        SLATE_ARGUMENT(bool, bReadOnly);
        SLATE_ARGUMENT(bool, bMultiSelect);
        SLATE_EVENT(FOnPropertySelected, OnPropertySelected);
    SLATE_END_ARGS();

    // SWidget
    void Construct(const FArguments& InArgs);
    // ~SWidget
    
    TSharedPtr<SWidget> GetWidgetToFocusOnOpen();

    /** Get the currently selected property */
    FLiveConfigProperty GetSelectedProperty() const { return SelectedProperty; }

    /** Set the selected property */
    void SetSelectedProperty(FLiveConfigProperty InProperty);

private:
    /** Refresh the list of available properties */
    void RefreshPropertyList();

    /** Filter the list based on search text */
    void OnFilterTextChanged(const FText& InFilterText);

    /** Handle selection in the list */
    void OnSelectionChanged(TSharedPtr<FLiveConfigProperty> InItem, ESelectInfo::Type SelectInfo);

    /** Handle adding a new property */
    FReply OnAddNewPropertyClicked();

    /** Get the display text for a property */
    FText GetPropertyDisplayText(FLiveConfigProperty Property) const;

    /** Get the description for a property */
    FText GetPropertyTooltipText(FLiveConfigProperty Property) const;

    /** Generate a row for the list view */
    TSharedRef<ITableRow> GenerateRow(TSharedPtr<FLiveConfigProperty> InItem, const TSharedRef<STableViewBase>& OwnerTable);

    FLiveConfigProperty SelectedProperty;
    FName Filter;
    TOptional<ELiveConfigPropertyType> FilterType;
    TWeakObjectPtr<UScriptStruct> StructFilter;
    bool bReadOnly = false;
    bool bMultiSelect = false;

    TArray<TSharedPtr<FLiveConfigProperty>> AvailablePropertyNames;
    TArray<TSharedPtr<FLiveConfigProperty>> FilteredProperties;
    TSharedPtr<class SSearchBox> SearchBox;
    TSharedPtr<class SListView<TSharedPtr<FLiveConfigProperty>>> PropertyListView;
    
    FOnPropertySelected OnPropertyChanged;
};

