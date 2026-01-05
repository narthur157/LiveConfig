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
	static bool IsFeatureEnabled(UPARAM(Meta=(Categories="LiveConfig")) FGameplayTag Row, bool bDefault);
	
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static float GetValue(UPARAM(Meta=(Categories="LiveConfig")) FGameplayTag Row, float DefaultValue);
	
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static FString GetStringValue(UPARAM(Meta=(Categories="LiveConfig")) FGameplayTag Row, const FString& DefaultString);
	
	static bool IsFeatureEnabled(FLiveConfigProperty Property, bool bDefault);
	
	static float GetValue(FLiveConfigProperty Property, float DefaultValue);
	
	static FString GetStringValue(FLiveConfigProperty Property, const FString& DefaultString);
	
	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FName GetPropertyName(const FLiveConfigProperty& Property);

	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FLiveConfigProperty MakeLiteralLiveConfigProperty(FLiveConfigProperty Property);
};
