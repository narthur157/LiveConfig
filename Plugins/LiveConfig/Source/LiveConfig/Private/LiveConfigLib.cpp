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

void ULiveConfigLib::GetStructValue(FLiveConfigProperty Property, int32& OutStruct)
{
	// This should not be called
	checkNoEntry();
}

void ULiveConfigLib::Generic_GetStructValue(FLiveConfigProperty Property, UScriptStruct* Struct, void* OutStructPtr)
{
	if (Struct && OutStructPtr)
	{
		if (ULiveConfigSystem* System = ULiveConfigSystem::Get())
		{
			System->GetLiveConfigStruct_Internal(Struct, OutStructPtr, Property);
		}
	}
}

DEFINE_FUNCTION(ULiveConfigLib::execGetStructValue)
{
	P_GET_STRUCT(FLiveConfigProperty, Property);

	Stack.StepCompiledIn<FStructProperty>(NULL);
	void* OutStructPtr = Stack.MostRecentPropertyAddress;
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);

	P_FINISH;

	if (StructProp && OutStructPtr)
	{
		P_NATIVE_BEGIN;
		Generic_GetStructValue(Property, StructProp->Struct, OutStructPtr);
		P_NATIVE_END;
	}
}

FName ULiveConfigLib::GetPropertyName(const FLiveConfigProperty& Property)
{
	return Property.GetName();
}

FLiveConfigProperty ULiveConfigLib::MakeLiteralLiveConfigProperty(FLiveConfigProperty Property)
{
	return Property;
}
