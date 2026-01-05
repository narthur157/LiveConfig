#include "LiveConfigPropertyPicker.h"

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
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLiveConfigPropertyPicker::Construct(const FArguments& InArgs)
{
    Filter = InArgs._Filter;
    bReadOnly = InArgs._bReadOnly;
    bMultiSelect = InArgs._bMultiSelect;
    OnPropertyChanged = InArgs._OnPropertyChanged;

    RefreshPropertyList();

    ChildSlot
    [
        SNew(SVerticalBox)
        
        // Search box
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SAssignNew(SearchBox, SSearchBox)
            .OnTextChanged(this, &SLiveConfigPropertyPicker::OnFilterTextChanged)
            .HintText(NSLOCTEXT("LiveConfig", "SearchProperties", "Search properties..."))
        ]

        // List of propertys
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(2.0f)
        [
            SAssignNew(PropertyListView, SListView<TSharedPtr<FLiveConfigProperty>>)
            .ListItemsSource(&FilteredProperties)
            .OnGenerateRow(this, &SLiveConfigPropertyPicker::GenerateRow)
            .OnSelectionChanged(this, &SLiveConfigPropertyPicker::OnSelectionChanged)
            .SelectionMode(bMultiSelect ? ESelectionMode::Multi : ESelectionMode::Single)
        ]

        // Separator
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SSeparator)
        ]

        // Add new property section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SHorizontalBox)
            
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SAssignNew(AddNewTextBox, SEditableTextBox)
                .HintText(NSLOCTEXT("LiveConfig", "AddNewProperty", "Enter new property name..."))
                .OnTextCommitted(this, &SLiveConfigPropertyPicker::OnCommitNewProperty)
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
                        OnCommitNewProperty(AddNewTextBox->GetText(), ETextCommit::OnEnter);
                    }
                    return FReply::Handled();
                })
                .IsEnabled(!bReadOnly)
            ]
        ]
    ];
}

void SLiveConfigPropertyPicker::RefreshPropertyList()
{
    AvailablePropertyNames.Empty();
    FilteredProperties.Empty();

    // Get all propertys from the system
    TArray<FLiveConfigProperty> AllProperties = ULiveConfigSystem::Get()->GetAllProperties();

    // Also get propertys from property definitions in editor settings
#if WITH_EDITOR
    if (ULiveConfigEditorSettings* EditorSettings = GetMutableDefault<ULiveConfigEditorSettings>())
    {
        EditorSettings->PropertyDefinitions.GetKeys(AllProperties);
    }
#endif

    // Remove duplicates and sort
    AllProperties.Sort([](const FLiveConfigProperty& A, const FLiveConfigProperty& B)
    {
        return A.ToString() < B.ToString();
    });

    for (const FLiveConfigProperty& Property : AllProperties)
    {
        if (Property.IsValid())
        {
            AvailablePropertyNames.Add(MakeShareable(new FLiveConfigProperty(Property.GetName())));
        }
    }

    // Apply current filter
    OnFilterTextChanged(SearchBox.IsValid() ? SearchBox->GetText() : FText::GetEmpty());
}

void SLiveConfigPropertyPicker::OnFilterTextChanged(const FText& InFilterText)
{
    FilteredProperties.Empty();

    const FString FilterString = InFilterText.ToString().ToLower();

    for (const TSharedPtr<FLiveConfigProperty>& Property : AvailablePropertyNames)
    {
        if (FilterString.IsEmpty() || Property->ToString().ToLower().Contains(FilterString))
        {
            FilteredProperties.Add(Property);
        }
    }

    if (PropertyListView.IsValid())
    {
        PropertyListView->RequestListRefresh();
    }
}

TSharedRef<ITableRow> SLiveConfigPropertyPicker::GenerateRow(TSharedPtr<FLiveConfigProperty> InItem, const TSharedRef<STableViewBase>& OwnerTable)
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
                .Text(GetPropertyDisplayText(*InItem))
                .ToolTipText(GetPropertyDescription(*InItem))
            ]
        ];
}

void SLiveConfigPropertyPicker::OnSelectionChanged(TSharedPtr<FLiveConfigProperty> InItem, ESelectInfo::Type SelectInfo)
{
    if (InItem.IsValid())
    {
        SelectedProperty = *InItem;
        OnPropertyChanged.ExecuteIfBound(SelectedProperty);
    }
}

void SLiveConfigPropertyPicker::OnAddNewProperty()
{
    if (AddNewTextBox.IsValid())
    {
        OnCommitNewProperty(AddNewTextBox->GetText(), ETextCommit::OnEnter);
    }
}

void SLiveConfigPropertyPicker::OnCommitNewProperty(const FText& InText, ETextCommit::Type CommitInfo)
{
    if (bReadOnly || CommitInfo != ETextCommit::OnEnter)
    {
        return;
    }

    const FString PropertyNameString = InText.ToString().TrimStartAndEnd();
    if (PropertyNameString.IsEmpty())
    {
        return;
    }

    FName NewPropertyName(*PropertyNameString);

    // Validate the name (basic validation)
    if (NewPropertyName == NAME_None)
    {
        return;
    }

    // Add to property definitions if in editor
#if WITH_EDITOR
    if (ULiveConfigEditorSettings* EditorSettings = GetMutableDefault<ULiveConfigEditorSettings>())
    {
        if (!EditorSettings->PropertyDefinitions.Contains(NewPropertyName))
        {
            FLiveConfigPropertyDefinition NewDef;
            NewDef.PropertyName = NewPropertyName;
            NewDef.Description = FString::Printf(TEXT("User-added property: %s"), *PropertyNameString);
            EditorSettings->PropertyDefinitions.Add(NewPropertyName, NewDef);
            EditorSettings->SaveConfig();
        }
    }
#endif

    // Refresh the list
    RefreshPropertyList();

    // Select the new item
    SelectedProperty = NewPropertyName;
    OnPropertyChanged.ExecuteIfBound(SelectedProperty);

    // Clear the text box
    if (AddNewTextBox.IsValid())
    {
        AddNewTextBox->SetText(FText::GetEmpty());
    }
}

FText SLiveConfigPropertyPicker::GetPropertyDisplayText(FLiveConfigProperty Property) const
{
    return FText::FromName(Property.GetName());
}

FText SLiveConfigPropertyPicker::GetPropertyDescription(FLiveConfigProperty Property) const
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
        if (const FLiveConfigPropertyDefinition* Def = EditorSettings->PropertyDefinitions.Find(Property))
        {
            return FText::FromString(Def->Description);
        }
    }
#endif

    return FText::GetEmpty();
}

void SLiveConfigPropertyPicker::SetSelectedProperty(FLiveConfigProperty InProperty)
{
    SelectedProperty = InProperty;

    // Find and select in list
    if (PropertyListView.IsValid())
    {
        for (const TSharedPtr<FLiveConfigProperty>& Property : FilteredProperties)
        {
            if (Property.IsValid() && *Property == InProperty)
            {
                PropertyListView->SetSelection(Property);
                break;
            }
        }
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
