// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigSettingsCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "IDesktopPlatform.h"
#include "IPythonScriptPlugin.h"
#include "Logging/MessageLog.h"
#include "Misc/Paths.h"
#include "Tools/SLiveConfigCleanupUnusedPropertiesWidget.h"
#include "Tools/SLiveConfigRedirectManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "LiveConfigSystem.h"

#define LOCTEXT_NAMESPACE "LiveConfigSettingsCustomization"

TSharedRef<IDetailCustomization> FLiveConfigSettingsCustomization::MakeInstance()
{
	return MakeShared<FLiveConfigSettingsCustomization>();
}

void FLiveConfigSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Utilities");

	Category.AddCustomRow(LOCTEXT("LiveConfigMaintenanceSearch", "Live Config Maintenance"))
		.WholeRowContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SyncRemoteToLocal", "Sync Remote to Local"))
					.ToolTipText(LOCTEXT("SyncRemoteToLocalTooltip", "Manually fetch remote overrides and apply them to local JSON files, regardless of the current Sync Mode setting."))
					.OnClicked(this, &FLiveConfigSettingsCustomization::OnSyncRemoteClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SyncRemoteToLocalDescription", "Fetches the latest remote config and updates local JSON files if changes are detected."))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CleanupUnusedProperties", "Cleanup Unused Properties"))
					.ToolTipText(LOCTEXT("CleanupUnusedPropertiesTooltip", "Review and optionally remove properties that are not used in any assets or config files"))
					.OnClicked(this, &FLiveConfigSettingsCustomization::OnCleanupUnusedPropertiesClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("CleanupUnusedPropertiesDescription", "Find properties not referenced by assets or config files and optionally remove them."))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ExportLiveConfigCsv", "Export CSV"))
					.ToolTipText(LOCTEXT("ExportLiveConfigCsvTooltip", "Export Live Config data using the ExportLiveConfig.py script"))
					.OnClicked(this, &FLiveConfigSettingsCustomization::OnExportCsvClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ExportLiveConfigCsvDescription", "Export the current Live Config JSON files to a CSV file for Google Sheets."))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ManageRedirects", "Manage Redirects"))
					.ToolTipText(LOCTEXT("ManageRedirectsTooltip", "View and clean up property redirects"))
					.OnClicked(this, &FLiveConfigSettingsCustomization::OnManageRedirectsClicked)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ManageRedirectsDescription", "Review or remove redirect entries created during Live Config property renames."))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		];
}

FReply FLiveConfigSettingsCustomization::OnCleanupUnusedPropertiesClicked()
{
	TSharedPtr<SWindow> CleanupWindow = SNew(SWindow)
		.Title(LOCTEXT("CleanupUnusedPropertiesWindowTitle", "Cleanup Unused Live Config Properties"))
		.ClientSize(FVector2D(600, 500))
		.SupportsMaximize(true)
		.SupportsMinimize(false)
		[
			SNew(SLiveConfigCleanupUnusedPropertiesWidget)
		];

	FSlateApplication::Get().AddWindow(CleanupWindow.ToSharedRef());
	return FReply::Handled();
}

FReply FLiveConfigSettingsCustomization::OnManageRedirectsClicked()
{
	TSharedPtr<SWindow> RedirectWindow = SNew(SWindow)
		.Title(LOCTEXT("ManageRedirectsWindowTitle", "Manage Property Redirects"))
		.ClientSize(FVector2D(900, 600))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SLiveConfigRedirectManager)
		];

	FSlateApplication::Get().AddWindow(RedirectWindow.ToSharedRef());
	return FReply::Handled();
}

FReply FLiveConfigSettingsCustomization::OnExportCsvClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> SaveFilenames;
	const bool bOpened = DesktopPlatform->SaveFileDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		LOCTEXT("ExportCsvDialogTitle", "Export Live Config to CSV").ToString(),
		FPaths::ProjectSavedDir(),
		TEXT("LiveConfig.csv"),
		TEXT("CSV files (*.csv)|*.csv"),
		EFileDialogFlags::None,
		SaveFilenames
	);

	if (!bOpened || SaveFilenames.Num() == 0)
	{
		return FReply::Handled();
	}

	IPythonScriptPlugin* PythonPlugin = IPythonScriptPlugin::Get();
	if (!PythonPlugin)
	{
		FMessageLog("LiveConfig").Error(LOCTEXT("PythonPluginMissing", "Python Script Plugin is not available. Enable it to export Live Config."));
		return FReply::Handled();
	}

	FString ScriptPath = FPaths::ConvertRelativePathToFull(
		FPaths::ProjectPluginsDir() / TEXT("LiveConfig/Scripts/ExportLiveConfig.py")
	);
	FString OutputPath = SaveFilenames[0];
	FPaths::NormalizeFilename(ScriptPath);
	FPaths::NormalizeFilename(OutputPath);

	const FString Command = FString::Printf(
		TEXT("import sys, runpy; sys.argv=[r'%s','--output',r'%s','--format','csv']; runpy.run_path(r'%s', run_name='__main__')"),
		*ScriptPath,
		*OutputPath,
		*ScriptPath
	);

	const bool bSuccess = PythonPlugin->ExecPythonCommand(*Command);
	if (bSuccess)
	{
		FMessageLog("LiveConfig").Info(FText::Format(
			LOCTEXT("ExportSuccess", "Successfully exported Live Config to {0}"),
			FText::FromString(OutputPath)
		));
	}
	else
	{
		FMessageLog("LiveConfig").Error(FText::Format(
			LOCTEXT("ExportFailed", "Failed to export Live Config to {0}. Check the Output Log for details."),
			FText::FromString(OutputPath)
		));
	}

	return FReply::Handled();
}

FReply FLiveConfigSettingsCustomization::OnSyncRemoteClicked()
{
	ULiveConfigSystem::Get().SyncRemoteToLocal();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
