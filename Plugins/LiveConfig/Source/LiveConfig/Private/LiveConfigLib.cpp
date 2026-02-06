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

FSlateColor ULiveConfigLib::GetTagColor(FName InTag)
{
	if (InTag.IsNone())
	{
		return FSlateColor(FLinearColor::Gray);
	}

	uint32 Hash = GetTypeHash(InTag.ToString());
	
	// Generate a color from hash
	float Hue = (Hash % 360);
	float Saturation = 0.8f;
	float Value = 0.6f;

	// Simple HSV to RGB
	auto HSVtoRGB = [](float h, float s, float v) -> FLinearColor
	{
		float c = v * s;
		float x = c * (1.0f - FMath::Abs(FMath::Fmod(h / 60.0f, 2.0f) - 1.0f));
		float m = v - c;
		float r, g, b;
		if (h < 60) { r = c; g = x; b = 0; }
		else if (h < 120) { r = x; g = c; b = 0; }
		else if (h < 180) { r = 0; g = c; b = x; }
		else if (h < 240) { r = 0; g = x; b = c; }
		else if (h < 300) { r = x; g = 0; b = c; }
		else { r = c; g = 0; b = x; }
		return FLinearColor(r + m, g + m, b + m, 1.0f);
	};

	return FSlateColor(HSVtoRGB(Hue, Saturation, Value));
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
