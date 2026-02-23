// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License


#include "LiveConfigLib.h"

#include "LiveConfigSettings.h"
#include "LiveConfigSystem.h"
#include "Serialization/Csv/CsvParser.h"
#include "JsonObjectConverter.h"

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

FLiveConfigProfile ULiveConfigLib::ParseOverridesFromCsv(const FString& CsvContent)
{
	FLiveConfigProfile Profile;
	FCsvParser Parser(CsvContent);
	const FCsvParser::FRows& Rows = Parser.GetRows();

	// Start from index 1 to skip the header row
	for (int32 i = 1; i < Rows.Num(); ++i)
	{
		const TArray<const TCHAR*>& Columns = Rows[i];

		// We expect at least two columns: key, value
		if (Columns.Num() >= 2)
		{
			const FName Key(Columns[0]);
			FLiveConfigProperty Property(Key, true);
			if (Property.IsValid())
			{
				Profile.Overrides.Add(Property, Columns[1]);
			}
		}
	}

	return Profile;
}

FSlateColor ULiveConfigLib::GetTagColor(FName InTag)
{
	if (InTag.IsNone())
	{
		return FSlateColor(FLinearColor::Gray);
	}

	uint32 Hash = GetTypeHash(InTag.ToString());
	
	// color from hash
	// at some point, tags might get the ability to customize colors in which case this would just be used for the default color
	float Hue = (Hash % 360);
	float Saturation = 0.8f;
	float Value = 0.6f;
	
	return FLinearColor(Hue, Saturation, Value).HSVToLinearRGB();
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

