// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigLib.h"

#include "LiveConfigGameSettings.h"
#include "LiveConfigSystem.h"

FLiveConfigPropertyDefinition ULiveConfigLib::GetLiveConfigPropertyDefinition(FLiveConfigProperty Property)
{
	if (!Property.IsValid())
	{
		return {};
	}
	const FLiveConfigPropertyDefinition* Prop = ULiveConfigSystem::Get()->PropertyDefinitions.Find(Property);
	
	if (!Prop)
	{
		return {};
	}
	
	return *Prop; 
}

bool ULiveConfigLib::IsFeatureEnabled(FLiveConfigProperty Property)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetBoolValue(Property);
	}

	return false;
}

float ULiveConfigLib::GetValue(FLiveConfigProperty Property)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetFloatValue(Property);
	}

	return 0.0f;
}

int32 ULiveConfigLib::GetIntValue(FLiveConfigProperty Property)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetIntValue(Property);
	}

	return 0;
}

FString ULiveConfigLib::GetStringValue(FLiveConfigProperty Property)
{
	if (auto LiveConfigSystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>())
	{
		return LiveConfigSystem->GetStringValue(Property);
	}

	return FString();
}

FName ULiveConfigLib::GetPropertyName(const FLiveConfigProperty& Property)
{
	return Property.GetName();
}

FLiveConfigProperty ULiveConfigLib::MakeLiteralLiveConfigProperty(FLiveConfigProperty Property)
{
	return Property;
}
