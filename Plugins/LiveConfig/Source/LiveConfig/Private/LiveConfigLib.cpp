// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigLib.h"

#include "GameplayTagContainer.h"
#include "LiveConfigSystem.h"

bool ULiveConfigLib::IsFeatureEnabled(FGameplayTag Row, bool bDefault)
{
	return IsFeatureEnabled(Row.GetTagName(), bDefault);
}

float ULiveConfigLib::GetValue(FGameplayTag Row, float DefaultValue)
{
	return GetValue(Row.GetTagName(), DefaultValue);
}

FString ULiveConfigLib::GetStringValue(FGameplayTag Row, const FString& DefaultString)
{
	return GetStringValue(Row.GetTagName(), DefaultString);
}

bool ULiveConfigLib::IsFeatureEnabled(FName RowName, bool bDefault)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetBoolValue(RowName, bDefault);
	}

	return bDefault;
}

float ULiveConfigLib::GetValue(FName RowName, float DefaultValue)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetFloatValue(RowName, DefaultValue);
	}

	return DefaultValue;
}

FString ULiveConfigLib::GetStringValue(FName RowName, const FString& DefaultString)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetStringValue(RowName, DefaultString);
	}

	return DefaultString;
}
