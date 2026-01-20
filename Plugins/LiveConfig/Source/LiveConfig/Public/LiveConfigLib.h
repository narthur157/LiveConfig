// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LiveConfigTypes.h"
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
	static FLiveConfigPropertyDefinition GetLiveConfigPropertyDefinition(FLiveConfigProperty Property);
	
	UFUNCTION(BlueprintPure, Category = "Live Config")
	static bool IsFeatureEnabled(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static float GetValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static int32 GetIntValue(FLiveConfigProperty Property);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static FString GetStringValue(FLiveConfigProperty Property);

	/**
	 * Gets a struct value from live config.
	 * Each property in the struct will be looked up as Property.PropertyName.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Live Config", meta = (CustomStructureParam = "OutStruct"))
	static void GetStructValue(FLiveConfigProperty Property, int32& OutStruct);

	static void Generic_GetStructValue(FLiveConfigProperty Property, UScriptStruct* Struct, void* OutStructPtr);

	DECLARE_FUNCTION(execGetStructValue);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static FName GetPropertyName(const FLiveConfigProperty& Property);

	UFUNCTION(BlueprintPure, Category = "Live Config")
	static FLiveConfigProperty MakeLiteralLiveConfigProperty(FLiveConfigProperty Property);
};
