#include "LiveConfigRowPicker.h"

#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "LiveConfigSystem.h"
#include "LiveConfigEditorSettings.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "EditorStyleSet.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLiveConfigRowPicker::Construct(const FArguments& InArgs)
{
    Filter = InArgs._Filter;
    bReadOnly = InArgs._bReadOnly;
    bMultiSelect = InArgs._bMultiSelect;
    OnRowNameChanged = InArgs._OnRowNameChanged;

    RefreshRowNameList();

    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Search box
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SAssignNew(SearchBox, SSearchBox)
            .OnTextChanged(this, &SLiveConfigRowPicker::OnFilterTextChanged)
            .HintText(NSLOCTEXT("LiveConfig", "SearchRowNames", "Search row names..."))
        ]

        // List of row names
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(2.0f)
        [
            SAssignNew(RowNameListView, SListView<TSharedPtr<FLiveConfigRowName>>)
            .ListItemsSource(&FilteredRowNames)
            .OnGenerateRow(this, &SLiveConfigRowPicker::GenerateRow)
            .OnSelectionChanged(this, &SLiveConfigRowPicker::OnSelectionChanged)
            .SelectionMode(bMultiSelect ? ESelectionMode::Multi : ESelectionMode::Single)
        ]

        // Separator
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SSeparator)
        ]

        // Add new row name section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(AddNewTextBox, SEditableTextBox)
                .HintText(NSLOCTEXT("LiveConfig", "AddNewRowName", "Enter new row name..."))
                .OnTextCommitted(this, &SLiveConfigRowPicker::OnCommitNewRowName)
                .IsEnabled(!bReadOnly)
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(4.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("LiveConfig", "Add", "Add"))
                .OnClicked_Lambda([this]()
                {
                    if (AddNewTextBox.IsValid())
                    {
                        OnCommitNewRowName(AddNewTextBox->GetText(), ETextCommit::OnEnter);
                    }
                    return FReply::Handled();
                })
                .IsEnabled(!bReadOnly)
            ]
        ]
    ];
}

void SLiveConfigRowPicker::RefreshRowNameList()
{
    AvailableRowNames.Empty();
    FilteredRowNames.Empty();

    // Get all row names from the system
    TArray<FLiveConfigRowName> AllRowNames = ULiveConfigSystem::Get()->GetAllRowNames();

    // Also get row names from property definitions in editor settings
#if WITH_EDITOR
    if (ULiveConfigEditorSettings* EditorSettings = GetMutableDefault<ULiveConfigEditorSettings>())
    {
        EditorSettings->PropertyDefinitions.GetKeys(AllRowNames);
    }
#endif

    // Remove duplicates and sort
    AllRowNames.Sort([](const FLiveConfigRowName& A, const FLiveConfigRowName& B)
    {
        return A.ToString() < B.ToString();
    });

    for (const FLiveConfigRowName& RowName : AllRowNames)
    {
        if (RowName.IsValid())
        {
            AvailableRowNames.Add(MakeShareable(new FLiveConfigRowName(RowName.GetRowName())));
        }
    }

    // Apply current filter
    OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

void SLiveConfigRowPicker::OnFilterTextChanged(const FText& InFilterText)
{
    FilteredRowNames.Empty();

    const FString FilterString = InFilterText.ToString().ToLower();

    for (const TSharedPtr<FLiveConfigRowName>& RowName : AvailableRowNames)
    {
        if (FilterString.IsEmpty() || RowName->ToString().ToLower().Contains(FilterString))
        {
            FilteredRowNames.Add(RowName);
        }
    }

    if (RowNameListView.IsValid())
    {
        RowNameListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SLiveConfigRowPicker::GenerateRow(TSharedPtr<FLiveConfigRowName> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FName>>, OwnerTable)
        .Content()
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(4.0f, 2.0f)
            [
                SNew(STextBlock)
                .Text(GetRowNameDisplayText(*InItem))
                .ToolTipText(GetRowNameDescription(*InItem))
            ]
        ];
}

void SLiveConfigRowPicker::OnSelectionChanged(TSharedPtr<FLiveConfigRowName> InItem, ESelectInfo::Type SelectInfo)
{
    if (InItem.IsValid())
    {
        SelectedRowName = *InItem;
        OnRowNameChanged.ExecuteIfBound(SelectedRowName);
    }
}

void SLiveConfigRowPicker::OnAddNewRowName()
{
    if (AddNewTextBox.IsValid())
    {
        OnCommitNewRowName(AddNewTextBox->GetText(), ETextCommit::OnEnter);
    }
}

void SLiveConfigRowPicker::OnCommitNewRowName(const FText& InText, ETextCommit::Type CommitInfo)
{
    if (bReadOnly || CommitInfo != ETextCommit::OnEnter)
    {
        return;
    }

    const FString NewRowNameString = InText.ToString().TrimStartAndEnd();
    if (NewRowNameString.IsEmpty())
    {
        return;
    }

    FName NewRowName(*NewRowNameString);

    // Validate the name (basic validation)
    if (NewRowName == NAME_None)
    {
        return;
    }

    // Add to property definitions if in editor
#if WITH_EDITOR
    if (ULiveConfigEditorSettings* EditorSettings = GetMutableDefault<ULiveConfigEditorSettings>())
    {
        if (!EditorSettings->PropertyDefinitions.Contains(NewRowName))
        {
            FLiveConfigPropertyDefinition NewDef;
            NewDef.PropertyName = NewRowName;
            NewDef.Description = FString::Printf(TEXT("User-added row name: %s"), *NewRowNameString);
            EditorSettings->PropertyDefinitions.Add(NewRowName, NewDef);
            EditorSettings->SaveConfig();
        }
    }
#endif

    // Refresh the list
    RefreshRowNameList();

    // Select the new item
    SelectedRowName = NewRowName;
    OnRowNameChanged.ExecuteIfBound(SelectedRowName);

    // Clear the text box
    if (AddNewTextBox.IsValid())
    {
        AddNewTextBox->SetText(FText::GetEmpty());
    }
}

FText SLiveConfigRowPicker::GetRowNameDisplayText(FLiveConfigRowName RowName) const
{
    return FText::FromName(RowName.GetRowName());
}

FText SLiveConfigRowPicker::GetRowNameDescription(FLiveConfigRowName RowName) const
{
    if (ULiveConfigSystem* System = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
    {
        // Try to get description from config values
        // Note: This would require making ConfigValues accessible or adding a getter
        // For now, return empty
    }

#if WITH_EDITOR
    if (const ULiveConfigEditorSettings* EditorSettings = GetDefault<ULiveConfigEditorSettings>())
    {
        if (const FLiveConfigPropertyDefinition* Def = EditorSettings->PropertyDefinitions.Find(RowName))
        {
            return FText::FromString(Def->Description);
        }
    }
#endif

    return FText::GetEmpty();
}

void SLiveConfigRowPicker::SetSelectedRowName(FLiveConfigRowName InRowName)
{
    SelectedRowName = InRowName;

    // Find and select in list
    if (RowNameListView.IsValid())
    {
        for (const TSharedPtr<FLiveConfigRowName>& RowName : FilteredRowNames)
        {
            if (RowName.IsValid() && *RowName == InRowName)
            {
                RowNameListView->SetSelection(RowName);
                break;
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
