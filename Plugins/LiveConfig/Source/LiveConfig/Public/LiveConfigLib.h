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
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static bool IsFeatureEnabled(FLiveConfigProperty Property);
	
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static float GetValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static int32 GetIntValue(FLiveConfigProperty Property);
	
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static FString GetStringValue(FLiveConfigProperty Property);
	
	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FName GetPropertyName(const FLiveConfigProperty& Property);

	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FLiveConfigProperty MakeLiteralLiveConfigProperty(FLiveConfigProperty Property);
};
