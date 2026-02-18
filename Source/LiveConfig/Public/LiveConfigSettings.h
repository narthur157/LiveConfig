// Fill out your copyright notice in the Description page of Project Settings.

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
	 * The URL for the remote override CSV. This CSV will be fetched and used to override key/value pairs
	 * 
	 * The first column is treated as "Name", and the second column is "Value" for each property
	 * For use with Google Sheets, ensure that this url ends with /export?format=csv in the format
	 * https://docs.google.com/spreadsheets/d/<spreadsheet_id>/export?format=csv
	 * 
	 * Sheets works up to about 10-20 CCU, beyond which your CSV must be provided by something like S3
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	FString RemoteOverrideCSVUrl;

	// default in-game polling rate, in seconds
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	float PollingRate = 30;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	bool bPollConfigInEditor = true;

	/**
	 * How often to refresh the config in editor
	 * This should be set to be relatively slow as it will happen even while the editor is idle
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	float EditorPollRateMinutes = 30;

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
