// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LiveConfigTypes.h"
#include "LiveConfigPropertyName.h"
#include "LiveConfigGameSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, DefaultConfig)
class LIVECONFIG_API ULiveConfigGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	virtual FName GetCategoryName() const override { return TEXT("Project"); }
	virtual FName GetSectionName() const override { return TEXT("Live Config"); }

	/** The URL for the public Google Sheet CSV. Set this in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	FString SheetUrl;

	// default in-game polling rate
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "GoogleSheets")
	float PollingRate = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> PropertyDefinitions;

	/** Global list of tags that can be assigned to properties. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "General")
	TArray<FName> KnownTags;

	/** The curve table to update with live config values (Export TO). */
	UPROPERTY(Config, EditAnywhere, Category = "CurveTable", Meta = (AllowedClasses = "/Script/Engine.CurveTable"))
	FSoftObjectPath ExportCurveTable;

	/** List of curve tables that provide live config values (Import FROM). */
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
};
