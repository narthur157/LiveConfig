// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigLib.h"

#include "LiveConfigSystem.h"

bool ULiveConfigLib::IsFeatureEnabled(FLiveConfigProperty Property, bool bDefault)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetBoolValue(Property, bDefault);
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

int32 ULiveConfigLib::GetIntValue(FLiveConfigProperty Property, int32 DefaultValue)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetIntValue(Property, DefaultValue);
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
