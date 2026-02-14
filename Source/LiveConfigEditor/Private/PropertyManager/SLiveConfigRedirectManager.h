#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SLiveConfigRedirectManager : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SLiveConfigRedirectManager, SCompoundWidget);
public:
	SLATE_BEGIN_ARGS(SLiveConfigRedirectManager)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void RefreshRedirectList();
	void OnCheckUsage(FName OldPropertyName);
	void OnRemoveRedirect(FName OldPropertyName);
	void OnCleanupUnusedRedirects();

	TSharedPtr<class SVerticalBox> RedirectListBox;
};
