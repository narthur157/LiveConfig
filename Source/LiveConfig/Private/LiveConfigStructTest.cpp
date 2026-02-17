// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigStructTest.h"
#include "LiveConfigSystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigStructLookupTest, "LiveConfig.System.StructLookup", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigStructLookupTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();

	// Backup original settings
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	// Setup test data
	FString Prefix = TEXT("TestStruct");
	
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(Prefix + TEXT(".SomeString"));
		Def.Value = TEXT("Hello Struct");
		Def.PropertyType = ELiveConfigPropertyType::String;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(Prefix + TEXT(".SomeInt"));
		Def.Value = TEXT("123");
		Def.PropertyType = ELiveConfigPropertyType::Int;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(Prefix + TEXT(".SomeFloat"));
		Def.Value = TEXT("456.78");
		Def.PropertyType = ELiveConfigPropertyType::Float;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(Prefix + TEXT(".SomeBool"));
		Def.Value = TEXT("true");
		Def.PropertyType = ELiveConfigPropertyType::Bool;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}

	// Rebuild cache for test data
	System.RebuildConfigCache();

	// Test struct lookup
	FLiveConfigTestStruct Result = System.GetLiveConfigStruct<FLiveConfigTestStruct>(FLiveConfigProperty(Prefix));

	TestEqual(TEXT("String property should match"), Result.SomeString, TEXT("Hello Struct"));
	TestEqual(TEXT("Int property should match"), Result.SomeInt, 123);
	TestNearlyEqual(TEXT("Float property should match"), Result.SomeFloat, 456.78f, 0.0001f);
	TestTrue(TEXT("Bool property should match"), Result.SomeBool);

	// Restore original settings
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();

	return true;
}
