#pragma once

#include "IDetailCustomization.h"

class FLiveConfigSettingsCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnCleanupUnusedPropertiesClicked();
	FReply OnManageRedirectsClicked();
	FReply OnExportCsvClicked();
};
