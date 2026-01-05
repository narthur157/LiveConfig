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

bool ULiveConfigLib::IsFeatureEnabled(FLiveConfigProperty Property, bool bDefault)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetBoolValue(Property.GetName(), bDefault);
	}

	return bDefault;
}

float ULiveConfigLib::GetValue(FLiveConfigProperty Property, float DefaultValue)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetFloatValue(Property, DefaultValue);
	}

	return DefaultValue;
}

FString ULiveConfigLib::GetStringValue(FLiveConfigProperty Property, const FString& DefaultString)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetStringValue(Property, DefaultString);
	}

	return DefaultString;
}

FName ULiveConfigLib::GetPropertyName(const FLiveConfigProperty& Property)
{
	return Property.GetName();
}

FLiveConfigProperty ULiveConfigLib::MakeLiteralLiveConfigProperty(FLiveConfigProperty Property)
{
	return Property;
}
