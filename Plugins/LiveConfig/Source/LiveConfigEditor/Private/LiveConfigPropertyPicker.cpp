#include "LiveConfigPropertyPicker.h"
#include "LiveConfigEditor/LiveConfigEditor.h"

#include "Framework/Application/SlateApplication.h"
#include "LiveConfigSystem.h"
#include "LiveConfigEditorSettings.h"
#include "LiveConfigGameSettings.h"
#include "LiveConfigLib.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/ComparisonUtility.h"

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
                .OnTextCommitted_Lambda([this](const FText& Text, ETextCommit::Type CommitType)
                {
                    if (CommitType == ETextCommit::OnEnter)
                    {
                        OnCommitNewProperty(Text, CommitType);
                    }
                })
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
    auto AllDefinitions = ULiveConfigSystem::Get()->GetAllProperties();
    
    TArray<FLiveConfigProperty> AllProperties;
    AllDefinitions.GetKeys(AllProperties);

    // Remove duplicates and sort
    AllProperties.Sort([](const FLiveConfigProperty& A, const FLiveConfigProperty& B)
    {
        return UE::ComparisonUtility::CompareNaturalOrder(A.ToString(), B.ToString()) < 0;
    });

    for (const FLiveConfigProperty& Property : AllProperties)
    {
        if (Property.IsValid())
        {
            if (FilterType.IsSet())
            {
                FLiveConfigPropertyDefinition Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(Property);
                if (!Def.IsValid())
                {
                    continue;
                }
                
                if (Def.PropertyType != FilterType.GetValue())
                {
                    continue;
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
                    SNew(SWidgetSwitcher)
                    .WidgetIndex_Lambda([InItem]()
                    {
                        if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
                        {
                            if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                            {
                                return Def->PropertyType == ELiveConfigPropertyType::Bool ? 1 : 0;
                            }
                        }
                        return 0;
                    })
                    + SWidgetSwitcher::Slot()
                    [
                        SNew(SEditableTextBox)
                        .Text_Lambda([InItem]()
                        {
                            if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
                            {
                                if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                                {
                                    return FText::FromString(Def->Value);
                                }
                            }
                            return FText::GetEmpty();
                        })
                        .OnVerifyTextChanged_Lambda([InItem](const FText& NewText, FText& OutError)
                        {
                            if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
                            {
                                if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                                {
                                    FString NewVal = NewText.ToString();
                                    if (Def->PropertyType == ELiveConfigPropertyType::Int)
                                    {
                                        if (NewVal.IsEmpty() || NewVal == TEXT("-")) return true;
                                        if (!NewVal.IsNumeric() || NewVal.Contains(TEXT(".")))
                                        {
                                            OutError = LOCTEXT("ValueIntError", "Value must be a valid integer.");
                                            return false;
                                        }
                                    }
                                    else if (Def->PropertyType == ELiveConfigPropertyType::Float)
                                    {
                                        if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) return true;
                                        if (!NewVal.IsNumeric())
                                        {
                                            OutError = LOCTEXT("ValueFloatError", "Value must be a valid number.");
                                            return false;
                                        }
                                    }
                                }
                            }
                            return true;
                        })
                        .OnTextCommitted_Lambda([InItem](const FText& NewText, ETextCommit::Type CommitType)
                        {
                            if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
                            {
                                if (FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                                {
                                    FString NewVal = NewText.ToString();
                                    bool bChanged = false;
                                    if (Def->PropertyType == ELiveConfigPropertyType::Int)
                                    {
                                        if (NewVal.IsEmpty() || NewVal == TEXT("-")) { Def->Value = "0"; bChanged = true; }
                                        else if (NewVal.IsNumeric() && !NewVal.Contains(TEXT("."))) { Def->Value = NewVal; bChanged = true; }
                                    }
                                    else if (Def->PropertyType == ELiveConfigPropertyType::Float)
                                    {
                                        if (NewVal.IsEmpty() || NewVal == TEXT("-") || NewVal == TEXT(".") || NewVal == TEXT("-.")) { Def->Value = "0"; bChanged = true; }
                                        else if (NewVal.IsNumeric()) { Def->Value = NewVal; bChanged = true; }
                                    }
                                    else
                                    {
                                        Def->Value = NewVal;
                                        bChanged = true;
                                    }

                                    if (bChanged)
                                    {
                                        System->SaveConfig();
                                        System->TryUpdateDefaultConfigFile();
                                        System->RefreshFromSettings();
                                    }
                                }
                            }
                        })
                    ]
                    + SWidgetSwitcher::Slot()
                    [
                        SNew(SCheckBox)
                        .IsChecked_Lambda([InItem]()
                        {
                            if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
                            {
                                if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                                {
                                    return Def->Value.ToBool() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                                }
                            }
                            return ECheckBoxState::Unchecked;
                        })
                        .OnCheckStateChanged_Lambda([InItem](ECheckBoxState NewState)
                        {
                            if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
                            {
                                if (FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
                                {
                                    Def->Value = NewState == ECheckBoxState::Checked ? TEXT("true") : TEXT("false");
                                    System->SaveConfig();
                                    System->TryUpdateDefaultConfigFile();
                                    System->RefreshFromSettings();
                                }
                            }
                        })
                    ]
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

    if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
    {
        if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(*InItem))
        {
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

                ContentBox->AddSlot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    TagsBox.ToSharedRef()
                ];
            }
        }
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
    if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
    {
        if (!System->PropertyDefinitions.Contains(NewPropertyName))
        {
            FLiveConfigPropertyDefinition NewDef;
            NewDef.PropertyName = NewPropertyName;
            NewDef.Description = FString::Printf(TEXT("User-added property: %s"), *PropertyNameString);
            
            if (FilterType.IsSet())
            {
                NewDef.PropertyType = FilterType.GetValue();
            }

            System->PropertyDefinitions.Add(NewPropertyName, NewDef);
            System->SaveConfig();
            System->TryUpdateDefaultConfigFile();

            System->RefreshFromSettings();
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
    if (const ULiveConfigSystem* System = ULiveConfigSystem::Get())
    {
        if (const FLiveConfigPropertyDefinition* Def = System->PropertyDefinitions.Find(Property))
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

void SLiveConfigPropertyPicker::PrivateRegisterAttributes(FSlateAttributeInitializer& AttributeInitializer)
{
}

SLATE_IMPLEMENT_WIDGET(SLiveConfigPropertyPicker);

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
