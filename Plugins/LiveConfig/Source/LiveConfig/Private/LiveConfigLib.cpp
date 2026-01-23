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
	const FLiveConfigPropertyDefinition* Prop = ULiveConfigSystem::Get().PropertyDefinitions.Find(Property);
	
	if (!Prop)
	{
		return {};
	}
	
	return *Prop; 
}

FName ULiveConfigLib::GetPropertyName(const FLiveConfigProperty& Property)
{
	return Property.GetName();
}

FLiveConfigProperty ULiveConfigLib::MakeLiteralLiveConfigProperty(FLiveConfigProperty Property)
{
	return Property;
}

bool ULiveConfigLib::GetBoolValue(FLiveConfigProperty Property)
{
	return ULiveConfigSystem::Get().GetBoolValue(Property);
}

float ULiveConfigLib::GetValue(FLiveConfigProperty Property)
{
	return ULiveConfigSystem::Get().GetFloatValue(Property);
}

int32 ULiveConfigLib::GetIntValue(FLiveConfigProperty Property)
{
	return ULiveConfigSystem::Get().GetIntValue(Property);
}

FString ULiveConfigLib::GetStringValue(FLiveConfigProperty Property)
{
	return ULiveConfigSystem::Get().GetStringValue(Property);
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
		ULiveConfigSystem::Get().GetLiveConfigStruct_Internal(Struct, OutStructPtr, Property);
	}
}

DEFINE_FUNCTION(ULiveConfigLib::execGetStructValue)
{
	P_GET_STRUCT(FLiveConfigProperty, Property);

	Stack.StepCompiledIn<FStructProperty>(nullptr);
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
