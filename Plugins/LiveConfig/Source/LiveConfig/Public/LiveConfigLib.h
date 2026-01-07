// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LiveConfigLib.generated.h"

/**
 * 
 */
UCLASS()
class LIVECONFIG_API ULiveConfigLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION()
	static bool IsFeatureEnabled(FLiveConfigProperty Property);

	UFUNCTION()
	static float GetValue(FLiveConfigProperty Property);

	UFUNCTION()
	static int32 GetIntValue(FLiveConfigProperty Property);

	UFUNCTION()
	static FString GetStringValue(FLiveConfigProperty Property);

	UFUNCTION()
	static FName GetPropertyName(const FLiveConfigProperty& Property);

	UFUNCTION()
	static FLiveConfigProperty MakeLiteralLiveConfigProperty(FLiveConfigProperty Property);
};
