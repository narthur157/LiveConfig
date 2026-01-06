#include "LiveConfigPropertyPicker.h"
#include "LiveConfigEditor/LiveConfigEditor.h"

#include "Framework/Application/SlateApplication.h"
#include "LiveConfigSystem.h"
#include "LiveConfigEditorSettings.h"
#include "LiveConfigGameSettings.h"
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

#define LOCTEXT_NAMESPACE "LiveConfigPropertyPicker"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLiveConfigPropertyPicker::Construct(const FArguments& InArgs)
{
    Filter = InArgs._Filter;
    FilterType = InArgs._FilterType;
    bReadOnly = InArgs._bReadOnly;
    bMultiSelect = InArgs._bMultiSelect;
    OnPropertyChanged = InArgs._OnPropertyChanged;

    RefreshPropertyList();

    ChildSlot
    [
        SNew(SVerticalBox)

        // Manager button
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .OnClicked_Lambda([]()
            {
                FLiveConfigEditorModule& EditorModule = FModuleManager::GetModuleChecked<FLiveConfigEditorModule>("LiveConfigEditor");
                EditorModule.OpenPropertyManager();
                return FReply::Handled();
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0, 0, 4, 0)
                [
                    SNew(SImage)
                    .Image(FAppStyle::GetBrush("Icons.Settings"))
                    .DesiredSizeOverride(FVector2D(16, 16))
                ]
                + SHorizontalBox::Slot()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(NSLOCTEXT("LiveConfig", "ManageProperties", "Manage Live Config..."))
                ]
            ]
        ]
        
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(2.0f)
        [
            SNew(SSeparator)
        ]

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

    // Get all propertys from the system - now consolidated
    TArray<FLiveConfigProperty> AllProperties = ULiveConfigSystem::Get()->GetAllProperties();

    // Remove duplicates and sort
    AllProperties.Sort([](const FLiveConfigProperty& A, const FLiveConfigProperty& B)
    {
        return A.ToString() < B.ToString();
    });

    for (const FLiveConfigProperty& Property : AllProperties)
    {
        if (Property.IsValid())
        {
            if (FilterType.IsSet())
            {
                if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
                {
                    if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(Property))
                    {
                        if (Def->PropertyType != FilterType.GetValue())
                        {
                            continue;
                        }
                    }
                }
            }
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
    TSharedPtr<SHorizontalBox> ContentBox;
    TSharedPtr<SHorizontalBox> TagsBox;
    FText ValueText = LOCTEXT("ValueUnknown", "Unknown");

    if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
    {
        if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(*InItem))
        {
            ValueText = FText::FromString(Def->Value);
            
            if (Def->Tags.Num() > 0)
            {
                SAssignNew(TagsBox, SHorizontalBox);
                for (const FName& PropertyTag : Def->Tags)
                {
                    TagsBox->AddSlot()
                    .AutoWidth()
                    .Padding(2, 0)
                    [
                        SNew(SBorder)
                        .BorderImage(FAppStyle::GetBrush("Graph.Node.Body"))
                        .BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f))
                        .Padding(FMargin(4, 0))
                        [
                            SNew(STextBlock)
                            .Text(FText::FromName(PropertyTag))
                            .Font(FAppStyle::GetFontStyle("SmallFont"))
                        ]
                    ];
                }
            }
        }
    }

    TSharedRef<STableRow<TSharedPtr<FLiveConfigProperty>>> Row = SNew(STableRow<TSharedPtr<FLiveConfigProperty>>, OwnerTable);
    
    Row->SetContent(
        SNew(SHorizontalBox)
        
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(4.0f, 2.0f)
        .VAlign(VAlign_Center)
        [
            SAssignNew(ContentBox, SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(SBox)
                .WidthOverride(150.0f)
                [
                    SNew(STextBlock)
                    .Text(GetPropertyDisplayText(*InItem))
                    .Font(FAppStyle::GetFontStyle("BoldFont"))
                    .ToolTipText(GetPropertyDescription(*InItem))
                ]
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(8.0f, 0.0f, 4.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(45.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("ValueLabel", "Value:"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
            ]

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(12.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(100.0f)
                [
                    SNew(STextBlock)
                    .Text(ValueText)
                ]
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(2.0f)
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .ToolTipText(NSLOCTEXT("LiveConfig", "GoToManager", "Go to Manager"))
            .OnClicked_Lambda([InItem]()
            {
                FLiveConfigEditorModule& EditorModule = FModuleManager::GetModuleChecked<FLiveConfigEditorModule>("LiveConfigEditor");
                EditorModule.OpenPropertyManager(*InItem);
                return FReply::Handled();
            })
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.Settings"))
                .DesiredSizeOverride(FVector2D(14, 14))
            ]
        ]
    );

    if (TagsBox.IsValid())
    {
        ContentBox->AddSlot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(8.0f, 0.0f, 0.0f, 0.0f)
        [
            TagsBox.ToSharedRef()
        ];
    }

    return Row;
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
    if (ULiveConfigGameSettings* GameSettings = GetMutableDefault<ULiveConfigGameSettings>())
    {
        if (!GameSettings->PropertyDefinitions.Contains(NewPropertyName))
        {
            FLiveConfigPropertyDefinition NewDef;
            NewDef.PropertyName = NewPropertyName;
            NewDef.Description = FString::Printf(TEXT("User-added property: %s"), *PropertyNameString);
            GameSettings->PropertyDefinitions.Add(NewPropertyName, NewDef);
            GameSettings->SaveConfig();
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
    if (const ULiveConfigGameSettings* GameSettings = GetDefault<ULiveConfigGameSettings>())
    {
        if (const FLiveConfigPropertyDefinition* Def = GameSettings->PropertyDefinitions.Find(Property))
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

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
