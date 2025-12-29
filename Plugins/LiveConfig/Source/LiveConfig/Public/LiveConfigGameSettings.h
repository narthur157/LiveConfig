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
	/** The URL for the public Google Sheet CSV. Set this in DefaultGame.ini. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite)
	FString SheetUrl;

	// default in-game polling rate
	UPROPERTY(Config)
	float PollingRate = 30;
};
