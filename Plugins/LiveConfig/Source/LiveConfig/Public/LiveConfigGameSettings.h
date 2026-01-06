// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
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
};
