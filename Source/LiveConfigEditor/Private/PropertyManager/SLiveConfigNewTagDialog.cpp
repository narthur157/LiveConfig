// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "SLiveConfigNewTagDialog.h"
#include "LiveConfigSystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "LiveConfigNewTagDialog"

void SLiveConfigNewTagDialog::Construct(const FArguments& InArgs)
{
	OnTagCreated = InArgs._OnTagCreated;

	ChildSlot
	[
		SNew(SBox)
		.Padding(16.0f)
		.MinDesiredWidth(300.0f)
		[
			SNew(SVerticalBox)
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TagNameLabel", "Tag Name"))
				.Font(FAppStyle::GetFontStyle("BoldFont"))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 16)
			[
				SAssignNew(TagNameTextBox, SEditableTextBox)
				.HintText(LOCTEXT("TagNameHint", "Enter tag name..."))
				.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType)
				{
					if (CommitType == ETextCommit::OnEnter)
					{
						HandleCreate();
					}
				})
			]

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
					.OnClicked(this, &SLiveConfigNewTagDialog::HandleCreate)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("CancelButton", "Cancel"))
					.OnClicked(this, &SLiveConfigNewTagDialog::OnCancelClicked)
				]
			]
		]
	];
}

bool SLiveConfigNewTagDialog::SupportsKeyboardFocus() const
{
	return true;
}

FReply SLiveConfigNewTagDialog::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		return HandleCreate();
	}
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnCancelClicked();
	}

	return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SLiveConfigNewTagDialog::OpenDialog(FOnTagCreated InOnTagCreated)
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("DialogTitle", "Create New Tag"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SLiveConfigNewTagDialog> Dialog = SNew(SLiveConfigNewTagDialog)
		.OnTagCreated(InOnTagCreated);

	Dialog->WindowPtr = Window;
	Window->SetContent(Dialog);

	Window->SetWidgetToFocusOnActivate(Dialog->TagNameTextBox);
	FSlateApplication::Get().AddModalWindow(Window, nullptr);
}

FReply SLiveConfigNewTagDialog::HandleCreate()
{
	FString TagStr = TagNameTextBox->GetText().ToString().TrimStartAndEnd();
	if (TagStr.IsEmpty())
	{
		return FReply::Handled();
	}

	FName NewTag(*TagStr);
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	if (!System.PropertyTags.Contains(NewTag))
	{
		System.PropertyTags.Add(NewTag);
		System.HandleTagsChanged();
		OnTagCreated.ExecuteIfBound(NewTag);
	}

	if (WindowPtr.IsValid())
	{
		WindowPtr.Pin()->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SLiveConfigNewTagDialog::OnCancelClicked()
{
	if (WindowPtr.IsValid())
	{
		WindowPtr.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
