// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LiveConfigTypes.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigSettings.generated.h"


UENUM()
enum class ELiveConfigRedirectMode : uint8
{
	/** Always create redirects automatically when renaming properties */
	AlwaysCreate,
	/** Never create redirects when renaming properties */
	NeverCreate,
	/** Prompt the user to choose whether to create redirects */
	Prompt
};

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig)
class LIVECONFIG_API ULiveConfigSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	virtual FName GetSectionName() const override { return TEXT("Live Config"); }

	/**
	 * Default source type for remote overrides (fallback if not set in user settings)
	 * - HttpCsv: Fetch from URL (e.g., Google Sheets)
	 * - LocalCsv: Load from Saved/LiveConfigOverrides.csv
	 * - None: No remote overrides
	 *
	 * Note: Per-user settings can be configured in Live Config User Settings
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "RemoteOverrides")
	ELiveConfigSourceType DefaultExternalSource = ELiveConfigSourceType::None;
	
	static FString GetSourcePath();

	/**
	 * Default source path for remote overrides (fallback if not set in user settings)
	 * The first column is treated as "Name", and the second column is "Value" for each property
	 *
	 * For HttpCsv type (Google Sheets):
	 * 1.) Dev env method, no delay. Up to 300 requests per minute total
	 *     Copy your Sheet URL and add "/export?format=csv" to it
	 *
	 * 2.) More scalable method (~2 minute delay before changes are consistent):
	 *     use File -> Share -> Publish to web. Pick the tab with your data and the CSV type
	 *     Ensure:
	 *     - "Automatically republish when changes are made" is enabled
	 *     - Type is set to Comma Separated Values
	 *     - Only the tab with your overrides is selected
	 *     Limits for this method are undocumented
	 *
	 * Note: This can be overridden per user or in-game
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "RemoteOverrides", Meta = (EditCondition = "DefaultExternalSource == ELiveConfigSourceType::HttpCsv", EditConditionHides))
	FString RemoteCsvUrl;

	/**
	 * Polling rate for packaged builds (in seconds)
	 * This controls how often the config is refreshed at runtime
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "RemoteOverrides", Meta = (ClampMin = "5.0", ClampMax = "3600.0"))
	float PollingRate = 30.0f;

	/** 
	 * The curve table to update with live config values (Export TO). 
	 * This is especially useful for FScalableFloat values in GAS - providing live config values to Gameplay Effects
	 */
	UPROPERTY(Config, EditAnywhere, Category = "CurveTable", Meta = (AllowedClasses = "/Script/Engine.CurveTable"))
	FSoftObjectPath ExportCurveTable;

	/** 
	 * List of curve tables that provide live config values (Import FROM). 
	 * These values are intended to be read-only in the live config, with the curve table acting as the source of truth
	 * If the curve table is modified, it will update the live config value
	 */
	UPROPERTY(Config, EditAnywhere, Category = "CurveTable", Meta = (AllowedClasses = "/Script/Engine.CurveTable"))
	TArray<FSoftObjectPath> ImportCurveTables;

	/** Whether to automatically create new rows in the ExportCurveTable for all Float and Int properties. */
	UPROPERTY(Config, EditAnywhere, Category = "CurveTable")
	bool bAutoCreateRowsInExportTable = true;

	/** Whether to enable profile replication via a spawned actor. */
	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	bool bEnableProfileReplication = true;

	/** The class of the actor used for profile replication. */
	UPROPERTY(Config, EditAnywhere, Category = "Profiles", Meta = (MetaClass = "/Script/LiveConfig.LiveConfigProfileActor"))
	FSoftClassPath ProfileActorClass;

	/**
	 * Controls how redirects are created when renaming properties in the Property Manager
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Property Manager")
	ELiveConfigRedirectMode RedirectMode = ELiveConfigRedirectMode::AlwaysCreate;
};
