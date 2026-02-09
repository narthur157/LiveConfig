#include "LiveConfigPropertyPicker.h"
#include "LiveConfigEditor/LiveConfigEditor.h"
#include "PropertyManager/SLiveConfigPropertyValueWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "LiveConfigSystem.h"
#include "PropertyManager/SLiveConfigNewPropertyDialog.h"
#include "LiveConfigSettings.h"
#include "LiveConfigLib.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Images/SImage.h"
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
    StructFilter = InArgs._StructFilter;
    bReadOnly = InArgs._bReadOnly;
    bMultiSelect = InArgs._bMultiSelect;
    OnPropertyChanged = InArgs._OnPropertySelected;

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
                EditorModule.OpenPropertyManager(FLiveConfigProperty(), true);
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

        // List of properties
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .MaxHeight(350)
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
            SNew(SButton)
            .HAlign(HAlign_Center)
            .OnClicked(this, &SLiveConfigPropertyPicker::OnAddNewPropertyClicked)
            .IsEnabled(!bReadOnly)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0, 0, 4, 0)
                [
                    SNew(SImage)
                    .Image(FAppStyle::GetBrush("EditableComboBox.Add"))
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(NSLOCTEXT("LiveConfig", "AddNewProperty", "Add New Property"))
                ]
            ]
        ]
    ];
}

TSharedPtr<SWidget> SLiveConfigPropertyPicker::GetWidgetToFocusOnOpen()
{
    return SearchBox;
}

void SLiveConfigPropertyPicker::RefreshPropertyList()
{
    AvailablePropertyNames.Empty();
    FilteredProperties.Empty();

   	auto AllDefinitions = ULiveConfigSystem::Get().GetAllProperties();
    
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

                if (Def.PropertyType == ELiveConfigPropertyType::Struct && StructFilter.IsValid())
                {
                    if (Def.Value != StructFilter->GetName())
                    {
                        continue;
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
    
    TSharedRef<STableRow<TSharedPtr<FLiveConfigProperty>>> Row = SNew(STableRow<TSharedPtr<FLiveConfigProperty>>, OwnerTable);
    
    // It may or may not be worth implementing the collapsible name functionality from the manager here
    // that being said...this widget is less about browsing properties and more about searching for exactly what you want
    
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
                .WidthOverride(350.0f)
                [
                    SNew(STextBlock)
                    .Text(GetPropertyDisplayText(*InItem))
                    .HighlightText_Lambda([&]
                    {
                       return Filter.IsValid() ? FText::FromName(Filter) : FText::GetEmpty(); 
                    })
                    .ColorAndOpacity_Lambda([&]
                    {
                       return FLinearColor::White; 
                    })
                    //.Font(FAppStyle::GetFontStyle("BoldFont"))
                    .ToolTipText(GetPropertyTooltipText(*InItem))
                ]
            ]
/*
 * Label for the value input - disabled to save clutter
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
*/

            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(12.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(100.0f)
                [
                    SNew(SLiveConfigPropertyValueWidget)
                    .Value_Lambda([InItem]()
                    {
                        if (const FLiveConfigPropertyDefinition* Def = ULiveConfigSystem::Get().PropertyDefinitions.Find(*InItem))
                        {
                            return Def->Value;
                        }
                        return FString();
                    })
                    .PropertyType_Lambda([InItem]()
                    {
                        if (const FLiveConfigPropertyDefinition* Def = ULiveConfigSystem::Get().PropertyDefinitions.Find(*InItem))
                        {
                            return Def->PropertyType;
                        }
                        return ELiveConfigPropertyType::String;
                    })
                    .OnValueChanged_Lambda([InItem](const FString& NewValue)
                    {
                        ULiveConfigSystem& System = ULiveConfigSystem::Get();
                        if (FLiveConfigPropertyDefinition* Def = System.PropertyDefinitions.Find(*InItem))
                        {
                            Def->Value = NewValue;
                            System.SaveProperty(*Def);
                        }
                    })
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
                EditorModule.OpenPropertyManager(*InItem, true);
                return FReply::Handled();
            })
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.Settings"))
                .DesiredSizeOverride(FVector2D(14, 14))
            ]
        ]
    );

    /*
     * List of tags on widget - disabled to save clutter and prevent jarring resizing while scrolling
    if (const FLiveConfigPropertyDefinition* Def = ULiveConfigSystem::Get().PropertyDefinitions.Find(*InItem))
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
    */

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

FReply SLiveConfigPropertyPicker::OnAddNewPropertyClicked()
{
    ELiveConfigPropertyType InitialType = FilterType.Get(ELiveConfigPropertyType::String);
    FString InitialName = SearchBox.IsValid() ? SearchBox->GetText().ToString() : TEXT("");

    SLiveConfigNewPropertyDialog::OpenDialog(InitialName, InitialType, FOnPropertyCreated::CreateLambda([this](const FLiveConfigPropertyDefinition& NewDef)
    {
        ULiveConfigSystem::Get().SaveProperty(NewDef);
        
        // Refresh the list
        RefreshPropertyList();

        // Select the new item
        SelectedProperty = NewDef.PropertyName;
        OnPropertyChanged.ExecuteIfBound(SelectedProperty);
    }));

    return FReply::Handled();
}

FText SLiveConfigPropertyPicker::GetPropertyDisplayText(FLiveConfigProperty Property) const
{
    return FText::FromName(Property.GetName());
}

FText SLiveConfigPropertyPicker::GetPropertyTooltipText(FLiveConfigProperty Property) const
{
    if (const FLiveConfigPropertyDefinition* Def = ULiveConfigSystem::Get().PropertyDefinitions.Find(Property))
    {
        return FText::FromString(Def->Description);
    }

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
                PropertyListView->RequestScrollIntoView(Property);
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

