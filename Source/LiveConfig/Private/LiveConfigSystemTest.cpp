#include "LiveConfigSystem.h"
#include "LiveConfigSettings.h"
#include "LiveConfigJson.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigRedirectTest, "LiveConfig.System.Redirects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigRedirectTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	// disable the json system so that we don't actually write files and to validate that these systems are cleanly separated
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	
	// Backup original state
	TMap<FName, FName> OriginalRedirects = System.PropertyRedirects;
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	
	System.PropertyRedirects.Empty();
	System.PropertyDefinitions.Empty();
	
	
	FName OriginalPropName = FName("Old.Property");
	FName NewPropName = FName("New.Property");
	FLiveConfigPropertyDefinition PropertyDefinition;
	PropertyDefinition.PropertyName = OriginalPropName;
	PropertyDefinition.PropertyType = ELiveConfigPropertyType::String;
	PropertyDefinition.Value = "TestValue";
	
	System.PropertyDefinitions.Add("Old.Property"_LC, PropertyDefinition);
	
	{
		FLiveConfigProperty Prop = OriginalPropName;
		System.RedirectPropertyName(Prop);
		TestEqual(TEXT("Property should not be redirected yet"), Prop.GetName(), OriginalPropName);
	}
	
	// basic redirect
	{
		System.RenameProperty( OriginalPropName, NewPropName, true);

		FLiveConfigProperty Prop = OriginalPropName;
		System.RedirectPropertyName(Prop);
		TestEqual(TEXT("Property should be redirected via function"), Prop.GetName(), NewPropName);
		
		FLiveConfigProperty PropViaConstructor(OriginalPropName, true);
		TestEqual(TEXT("Property should be redirected via constructor"), PropViaConstructor.GetName(), NewPropName);
		
		FLiveConfigProperty PropViaLiteral = "Old.Property"_LC;
		TestEqual(TEXT("Property should be redirected via literal"), PropViaLiteral.GetName(), NewPropName);
	}

	// Test circular redirect (should break after MaxRedirects)
	{
		System.PropertyRedirects.Add(TEXT("Circular.A"), TEXT("Circular.B"));
		System.PropertyRedirects.Add(TEXT("Circular.B"), TEXT("Circular.A"));
		FLiveConfigProperty CircularProp(TEXT("Circular.A"));
		
		AddExpectedError("Circular Redirect", EAutomationExpectedMessageFlags::Contains);
		
		System.RedirectPropertyName(CircularProp);
		
		// It should still be one of them, but not crash
		TestTrue(TEXT("Circular redirect should not crash"), CircularProp.GetName() == FName(TEXT("Circular.A")) || CircularProp.GetName() == FName(TEXT("Circular.B")));
	}

	TestTrue(TEXT("Property should be stored under new name"), System.PropertyDefinitions.Contains(FLiveConfigProperty(NewPropName)));
	TestFalse(TEXT("Property should NOT be stored under old name"), System.PropertyDefinitions.Contains(FLiveConfigProperty(OriginalPropName)));

	// Restore original state
	System.PropertyRedirects = OriginalRedirects;
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();

	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigRenameTest, "LiveConfig.System.Rename", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigRenameTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	// Backup original state
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	// 1. Test Simple Rename
	{
		FLiveConfigProperty OldName(TEXT("Rename.Old"));
		FLiveConfigProperty NewName(TEXT("Rename.New"));

		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = OldName;
		Def.Value = TEXT("OriginalValue");
		Def.PropertyType = ELiveConfigPropertyType::String;
		System.PropertyDefinitions.Add(OldName, Def);
		System.RebuildConfigCache();

		System.RenameProperty(OldName, NewName);

		TestFalse(TEXT("Old property should be removed from definitions"), System.PropertyDefinitions.Contains(OldName));
		TestTrue(TEXT("New property should be in definitions"), System.PropertyDefinitions.Contains(NewName));
		TestEqual(TEXT("Value should be preserved"), System.GetStringValue(NewName), TEXT("OriginalValue"));
	}

	// 2. Test Struct Rename (including members)
	{
		FLiveConfigProperty OldStructName(TEXT("Rename.OldStruct"));
		FLiveConfigProperty NewStructName(TEXT("Rename.NewStruct"));

		// Define Struct
		FLiveConfigPropertyDefinition StructDef;
		StructDef.PropertyName = OldStructName;
		StructDef.PropertyType = ELiveConfigPropertyType::Struct;
		System.PropertyDefinitions.Add(OldStructName, StructDef);

		// Define Members
		FLiveConfigProperty OldMemberAName(TEXT("Rename.OldStruct.MemberA"));
		FLiveConfigProperty OldMemberBName(TEXT("Rename.OldStruct.MemberB"));

		FLiveConfigPropertyDefinition MemberADef;
		MemberADef.PropertyName = OldMemberAName;
		MemberADef.Value = TEXT("1.0");
		MemberADef.PropertyType = ELiveConfigPropertyType::Float;
		System.PropertyDefinitions.Add(OldMemberAName, MemberADef);

		FLiveConfigPropertyDefinition MemberBDef;
		MemberBDef.PropertyName = OldMemberBName;
		MemberBDef.Value = TEXT("42");
		MemberBDef.PropertyType = ELiveConfigPropertyType::Int;
		System.PropertyDefinitions.Add(OldMemberBName, MemberBDef);

		System.RebuildConfigCache();

		// Perform Rename
		System.RenameProperty(OldStructName, NewStructName);

		// Verify Struct Rename
		TestFalse(TEXT("Old struct should be removed"), System.PropertyDefinitions.Contains(OldStructName));
		TestTrue(TEXT("New struct should be added"), System.PropertyDefinitions.Contains(NewStructName));

		// Verify Members Rename
		FLiveConfigProperty NewMemberAName(TEXT("Rename.NewStruct.MemberA"));
		FLiveConfigProperty NewMemberBName(TEXT("Rename.NewStruct.MemberB"));

		TestFalse(TEXT("Old MemberA should be removed"), System.PropertyDefinitions.Contains(OldMemberAName));
		TestTrue(TEXT("New MemberA should be added"), System.PropertyDefinitions.Contains(NewMemberAName));
		TestNearlyEqual(TEXT("MemberA value should be preserved"), System.GetFloatValue(NewMemberAName), 1.0f, 0.001f);

		TestFalse(TEXT("Old MemberB should be removed"), System.PropertyDefinitions.Contains(OldMemberBName));
		TestTrue(TEXT("New MemberB should be added"), System.PropertyDefinitions.Contains(NewMemberBName));
		TestEqual(TEXT("MemberB value should be preserved"), System.GetIntValue(NewMemberBName), 42);
	}

	// 3. Test Rename with Redirector
	{
		TMap<FName, FName> OriginalRedirects = System.PropertyRedirects;
		System.PropertyRedirects.Empty();

		FLiveConfigProperty OldName(TEXT("Rename.WithRedirect.Old"));
		FLiveConfigProperty NewName(TEXT("Rename.WithRedirect.New"));

		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = OldName;
		Def.Value = TEXT("RedirectValue");
		Def.PropertyType = ELiveConfigPropertyType::String;
		System.PropertyDefinitions.Add(OldName, Def);
		System.RebuildConfigCache();

		System.RenameProperty(OldName, NewName, true);

		TestTrue(TEXT("Redirector should be created"), System.PropertyRedirects.Contains(OldName.GetName()));
		TestEqual(TEXT("Redirector should point to new name"), System.PropertyRedirects[OldName.GetName()], NewName.GetName());

		// Test retrieval via old name (should work because of redirection)
		TestEqual(TEXT("Should be able to get value using old name"), System.GetStringValue(OldName), TEXT("RedirectValue"));

		System.PropertyRedirects = OriginalRedirects;
	}

	// 4. Test Struct Rename with Redirector
	{
		TMap<FName, FName> OriginalRedirects = System.PropertyRedirects;
		System.PropertyRedirects.Empty();

		FLiveConfigProperty OldStructName(TEXT("Rename.RedirectStruct.Old"));
		FLiveConfigProperty NewStructName(TEXT("Rename.RedirectStruct.New"));

		// Define Struct
		FLiveConfigPropertyDefinition StructDef;
		StructDef.PropertyName = OldStructName;
		StructDef.PropertyType = ELiveConfigPropertyType::Struct;
		System.PropertyDefinitions.Add(OldStructName, StructDef);

		// Define Member
		FLiveConfigProperty OldMemberName(TEXT("Rename.RedirectStruct.Old.Member"));
		FLiveConfigPropertyDefinition MemberDef;
		MemberDef.PropertyName = OldMemberName;
		MemberDef.Value = TEXT("StructRedirectValue");
		MemberDef.PropertyType = ELiveConfigPropertyType::String;
		System.PropertyDefinitions.Add(OldMemberName, MemberDef);

		System.RebuildConfigCache();

		System.RenameProperty(OldStructName, NewStructName, true);

		// Verify Redirectors
		TestTrue(TEXT("Struct redirector should be created"), System.PropertyRedirects.Contains(OldStructName.GetName()));
		
		FLiveConfigProperty NewMemberName(TEXT("Rename.RedirectStruct.New.Member"));
		TestTrue(TEXT("Member redirector should be created"), System.PropertyRedirects.Contains(OldMemberName.GetName()));
		TestEqual(TEXT("Member redirector should point to new member name"), System.PropertyRedirects[OldMemberName.GetName()], NewMemberName.GetName());

		// Test retrieval via old member name
		TestEqual(TEXT("Should be able to get member value using old name"), System.GetStringValue(OldMemberName), TEXT("StructRedirectValue"));

		System.PropertyRedirects = OriginalRedirects;
	}

	// Restore original state
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();

	return true;
}

