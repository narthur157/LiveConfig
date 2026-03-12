// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LiveConfigTypes.h"
#include "LiveConfigUserSettings.generated.h"

/**
 * User-specific settings for LiveConfig remote sync behavior
 * These settings are per-user, not project-wide
 */
UCLASS(Config=EditorPerProjectUserSettings, DefaultConfig)
class LIVECONFIG_API ULiveConfigUserSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("General"); }
	virtual FName GetSectionName() const override { return TEXT("Live Config User Settings"); }

	/**
	 * Controls how external changes are synced to local property definitions in the editor
	 * Default: NeverSync
	 */
	UPROPERTY(Config, EditAnywhere, Category = "ExternalOverrides")
	ELiveConfigSyncMode SyncMode;
	
	static FString GetSourcePath();

	/**
	 * Source type for remote overrides
	 * - HttpCsv: Fetch from URL (e.g., Google Sheets)
	 * - LocalCsv: Load from local file
	 * - None: No remote overrides
	 */
	UPROPERTY(Config, EditAnywhere, Category = "ExternalOverrides")
	ELiveConfigSourceType ExternalSourceOverride = ELiveConfigSourceType::None;

	/**
	 * Source path for remote overrides
	 * - For HttpCsv: URL to CSV file (e.g., https://docs.google.com/spreadsheets/.../export?format=csv)
	 */
	UPROPERTY(Config, EditAnywhere, Category = "ExternalOverrides", Meta = (EditCondition = "ExternalSourceOverride == ELiveConfigSourceType::HttpCsv", EditConditionHides))
	FString RemoveCsvUrl;

	/**
	 * Local CSV that can be used to override properties in editor
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "ExternalOverrides", Meta = (EditCondition = "ExternalSourceOverride == ELiveConfigSourceType::LocalCsv", EditConditionHides, FilePathFilter = "csv"))
	FFilePath LocalCSVPath;

	/**
	 * How often to refresh the config in editor (in minutes)
	 * This polling happens even while the editor is idle
	 */
	UPROPERTY(Config, EditAnywhere, Category = "ExternalOverrides", Meta = (ClampMin = "1.0", ClampMax = "1440.0"))
	float EditorPollRateMinutes = 30.0f;
};
