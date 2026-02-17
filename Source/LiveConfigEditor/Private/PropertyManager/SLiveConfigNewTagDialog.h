// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FOnTagCreated, FName);

class SLiveConfigNewTagDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLiveConfigNewTagDialog)
	{}
		SLATE_EVENT(FOnTagCreated, OnTagCreated);
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	static void OpenDialog(FOnTagCreated InOnTagCreated);
	
private:
	FReply HandleCreate();
	FReply OnCancelClicked();
	
	TSharedPtr<class SEditableTextBox> TagNameTextBox;
	FOnTagCreated OnTagCreated;
	TWeakPtr<SWindow> WindowPtr;
};
