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
	
	static bool IsFeatureEnabled(FName RowName, bool bDefault);
	
	static float GetValue(FName RowName, float DefaultValue);
	
	static FString GetStringValue(FName RowName, const FString& DefaultString);
	
	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FName GetRowName(const FLiveConfigRowName& RowName);

	UFUNCTION(BlueprintPure, Category = "Live Config", meta = (BlueprintThreadSafe))
	static FLiveConfigRowName MakeLiteralLiveConfigRowName(FLiveConfigRowName RowName);
};
