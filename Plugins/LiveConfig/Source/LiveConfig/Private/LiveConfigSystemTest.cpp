#include "LiveConfigSystem.h"
#include "LiveConfigGameSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigSystemValueTest, "LiveConfig.System.Values", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigSystemValueTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	// Backup original settings
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	// Setup test data
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("Test.String")));
		Def.Value = TEXT("Hello World");
		Def.PropertyType = ELiveConfigPropertyType::String;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("Test.Int")));
		Def.Value = TEXT("42");
		Def.PropertyType = ELiveConfigPropertyType::Int;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("Test.Float")));
		Def.Value = TEXT("3.14");
		Def.PropertyType = ELiveConfigPropertyType::Float;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("Test.BoolTrue")));
		Def.Value = TEXT("true");
		Def.PropertyType = ELiveConfigPropertyType::Bool;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("Test.BoolFalse")));
		Def.Value = TEXT("false");
		Def.PropertyType = ELiveConfigPropertyType::Bool;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	}

	// Rebuild cache for test data
	System.RebuildConfigCache();

	// Test valid keys
	TestEqual(TEXT("String value should match"), System.GetStringValue(FLiveConfigProperty(FName(TEXT("Test.String")))), TEXT("Hello World"));
	TestEqual(TEXT("Int value should match"), System.GetIntValue(FLiveConfigProperty(FName(TEXT("Test.Int")))), 42);
	TestNearlyEqual(TEXT("Float value should match"), System.GetFloatValue(FLiveConfigProperty(FName(TEXT("Test.Float")))), 3.14f, 0.0001f);
	TestTrue(TEXT("Bool true value should match"), System.GetBoolValue(FLiveConfigProperty(FName(TEXT("Test.BoolTrue")))));
	TestFalse(TEXT("Bool false value should match"), System.GetBoolValue(FLiveConfigProperty(FName(TEXT("Test.BoolFalse")))));

	// Test missing keys
	TestEqual(TEXT("Missing string should be empty"), System.GetStringValue(FLiveConfigProperty(FName(TEXT("Missing")))), TEXT(""));
	TestEqual(TEXT("Missing int should be 0"), System.GetIntValue(FLiveConfigProperty(FName(TEXT("Missing")))), 0);
	TestEqual(TEXT("Missing float should be 0.0"), System.GetFloatValue(FLiveConfigProperty(FName(TEXT("Missing")))), 0.0f);
	TestFalse(TEXT("Missing bool should be false"), System.GetBoolValue(FLiveConfigProperty(FName(TEXT("Missing")))));

	// Test type mismatches / invalid formats
	TestEqual(TEXT("Int from string should be 0"), System.GetIntValue(FLiveConfigProperty(FName(TEXT("Test.String")))), 0);
	TestEqual(TEXT("Float from string should be 0.0"), System.GetFloatValue(FLiveConfigProperty(FName(TEXT("Test.String")))), 0.0f);
	TestFalse(TEXT("Bool from string should be false"), System.GetBoolValue(FLiveConfigProperty(FName(TEXT("Test.String")))));
	
	// Restore original settings
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();

	return true;
}
