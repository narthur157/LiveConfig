#include "SLiveConfigNewPropertyDialog.h"

#include "LiveConfigGameSettings.h"
#include "SLiveConfigTagPicker.h"
#include "SLiveConfigTagRow.h"
#include "PropertyManager/SLiveConfigPropertyValueWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SWrapBox.h"

#define LOCTEXT_NAMESPACE "SLiveConfigNewPropertyDialog"

void SLiveConfigNewPropertyDialog::Construct(const FArguments& InArgs)
{
	OnPropertyCreated = InArgs._OnPropertyCreated;
	SelectedType = InArgs._InitialType;

	TempDefinition = MakeShared<FLiveConfigPropertyDefinition>();
	TempDefinition->PropertyType = SelectedType;

	TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::String));
	TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Int));
	TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Float));
	TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Bool));
	TypeOptions.Add(MakeShared<ELiveConfigPropertyType>(ELiveConfigPropertyType::Struct));

	ChildSlot
	[
		SNew(SBox)
		.Padding(16.0f)
		.MinDesiredWidth(400.0f)
		[
			SNew(SVerticalBox)
			
			// Name
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NameLabel", "Property Name"))
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(NameTextBox, SEditableTextBox)
					.Text(FText::FromString(InArgs._InitialName))
					.HintText(LOCTEXT("NameHint", "e.g. MyCategory.MyProperty"))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							HandleCreate();
						}
					})
				]
			]

			// Type
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TypeLabel", "Property Type"))
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TypeComboBox, SComboBox<TSharedPtr<ELiveConfigPropertyType>>)
					.OptionsSource(&TypeOptions)
					.OnGenerateWidget(this, &SLiveConfigNewPropertyDialog::OnGenerateTypeWidget)
					.OnSelectionChanged(this, &SLiveConfigNewPropertyDialog::OnTypeSelected)
					[
						SNew(STextBlock)
						.Text(this, &SLiveConfigNewPropertyDialog::GetSelectedTypeText)
					]
				]
			]

			// Value
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ValueLabel", "Initial Value"))
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SLiveConfigPropertyValueWidget)
					.PropertyType_Lambda([this](){ return SelectedType; })
					.Value_Lambda([this](){ return TempDefinition->Value; })
					.OnValueChanged_Lambda([this](const FString& NewValue){ TempDefinition->Value = NewValue; })
					.OnEnter_Lambda([this](){ HandleCreate(); })
				]
			]

			// Description
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("DescriptionLabel", "Description"))
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(DescriptionTextBox, SEditableTextBox)
					.HintText(LOCTEXT("DescriptionHint", "Optional description..."))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							HandleCreate();
						}
					})
				]
			]

			// Tags
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 16)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TagsLabel", "Tags"))
					.Font(FAppStyle::GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SComboButton)
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.HasDownArrow(false)
						.ContentPadding(FMargin(4.0f, 2.0f))
						.OnGetMenuContent_Lambda([this]()
						{
							return SNew(SLiveConfigTagPicker)
								.TagVisibilityFilter([this](FName InTag)
								{
									return !TempDefinition->Tags.Contains(InTag);
								})
								.OnTagSelected(this, &SLiveConfigNewPropertyDialog::OnTagSelected);
						})
						.ButtonContent()
						[
							SNew(SBorder)
							.Padding(FMargin(8, 2))
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(FLinearColor(0.2f, 0.2f, 0.2f, 0.5f))
							[
								SNew(STextBlock)
								.Text(LOCTEXT("AddTagLabel", "+ Tag"))
								.Font(FAppStyle::GetFontStyle("NormalFont"))
								.ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					[
						SAssignNew(TagWrapBox, SWrapBox)
					]
				]
			]

			// Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(4, 0))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CreateButton", "Create"))
					.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
					.OnClicked(this, &SLiveConfigNewPropertyDialog::HandleCreate)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelButton", "Cancel"))
					.OnClicked(this, &SLiveConfigNewPropertyDialog::OnCancelClicked)
				]
			]
		]
	];

	// Initialize combo box selection
	for (auto& Option : TypeOptions)
	{
		if (*Option == SelectedType)
		{
			TypeComboBox->SetSelectedItem(Option);
			break;
		}
	}
	
	RefreshTags();
}

bool SLiveConfigNewPropertyDialog::SupportsKeyboardFocus() const
{
	return true;
}

FReply SLiveConfigNewPropertyDialog::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		return HandleCreate();
	}
	else if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnCancelClicked();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SLiveConfigNewPropertyDialog::OpenDialog(const FString& InInitialName, ELiveConfigPropertyType InInitialType, FOnPropertyCreated InOnPropertyCreated)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "Create New Live Config Property"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SLiveConfigNewPropertyDialog> Dialog = SNew(SLiveConfigNewPropertyDialog)
		.InitialName(InInitialName)
		.InitialType(InInitialType)
		.OnPropertyCreated(InOnPropertyCreated);

	Dialog->WindowPtr = Window;
	Window->SetContent(Dialog);

	Window->SetWidgetToFocusOnActivate(Dialog->NameTextBox);
	FSlateApplication::Get().AddModalWindow(Window, nullptr);
}

FReply SLiveConfigNewPropertyDialog::HandleCreate()
{
	FString NameStr = NameTextBox->GetText().ToString().TrimStartAndEnd();
	if (NameStr.IsEmpty())
	{
		return FReply::Handled();
	}

	FLiveConfigProperty Name(*NameStr);
	if (!Name.IsValid() || NameStr.EndsWith(TEXT(".")))
	{
		return FReply::Handled();
	}

	TempDefinition->PropertyName = Name;
	TempDefinition->PropertyType = SelectedType;
	TempDefinition->Description = DescriptionTextBox->GetText().ToString();

	OnPropertyCreated.ExecuteIfBound(*TempDefinition);

	if (WindowPtr.IsValid())
	{
		WindowPtr.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SLiveConfigNewPropertyDialog::OnCancelClicked()
{
	if (WindowPtr.IsValid())
	{
		WindowPtr.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

void SLiveConfigNewPropertyDialog::OnTypeSelected(TSharedPtr<ELiveConfigPropertyType> NewType, ESelectInfo::Type SelectInfo)
{
	if (NewType.IsValid())
	{
		SelectedType = *NewType;
		TempDefinition->PropertyType = SelectedType;
		// Reset value when type changes to avoid invalid data
		TempDefinition->Value = FString();
	}
}

TSharedRef<SWidget> SLiveConfigNewPropertyDialog::OnGenerateTypeWidget(TSharedPtr<ELiveConfigPropertyType> InType)
{
	FText TypeText;
	switch (*InType)
	{
	case ELiveConfigPropertyType::String: TypeText = LOCTEXT("TypeString", "String"); break;
	case ELiveConfigPropertyType::Int:    TypeText = LOCTEXT("TypeInt", "Int"); break;
	case ELiveConfigPropertyType::Float:  TypeText = LOCTEXT("TypeFloat", "Float"); break;
	case ELiveConfigPropertyType::Bool:   TypeText = LOCTEXT("TypeBool", "Bool"); break;
	case ELiveConfigPropertyType::Struct: TypeText = LOCTEXT("TypeStruct", "Struct"); break;
	}

	return SNew(STextBlock).Text(TypeText);
}

FText SLiveConfigNewPropertyDialog::GetSelectedTypeText() const
{
	switch (SelectedType)
	{
	case ELiveConfigPropertyType::String: return LOCTEXT("TypeString", "String");
	case ELiveConfigPropertyType::Int:    return LOCTEXT("TypeInt", "Int");
	case ELiveConfigPropertyType::Float:  return LOCTEXT("TypeFloat", "Float");
	case ELiveConfigPropertyType::Bool:   return LOCTEXT("TypeBool", "Bool");
	case ELiveConfigPropertyType::Struct: return LOCTEXT("TypeStruct", "Struct");
	}
	return FText::GetEmpty();
}

void SLiveConfigNewPropertyDialog::OnTagSelected(FName TagName)
{
	if (!TempDefinition->Tags.Contains(TagName))
	{
		TempDefinition->Tags.Add(TagName);
	}
	else
	{
		TempDefinition->Tags.Remove(TagName);
	}

	RefreshTags();
}

void SLiveConfigNewPropertyDialog::RefreshTags()
{
	if (TagWrapBox.IsValid())
	{
		TagWrapBox->ClearChildren();
		for (int32 i = 0; i < TempDefinition->Tags.Num(); ++i)
		{
			TagWrapBox->AddSlot()
			.Padding(2.0f)
			[
				SNew(SLiveConfigTagRow, TempDefinition, i, FSimpleDelegate::CreateSP(this, &SLiveConfigNewPropertyDialog::RefreshTags))
			];
		}
	}
	
}

#undef LOCTEXT_NAMESPACE
