// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigSystem.h"
#include "LiveConfigSettings.h"
#include "LiveConfigJson.h"
#include "Profiles/LiveConfigProfileSystem.h"
#include "Profiles/LiveConfigProfile.h"
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
		TestEqual(TEXT("Should be able to get value using old name"), System.GetStringValue(FLiveConfigProperty(OldName.GetName(), true)), TEXT("RedirectValue"));

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
		bool bMemberRedirectExists = System.PropertyRedirects.Contains(OldMemberName.GetName());
		TestTrue(TEXT("Member redirector should be created"), bMemberRedirectExists);
		if (bMemberRedirectExists)
		{
			TestEqual(TEXT("Member redirector should point to new member name"), System.PropertyRedirects[OldMemberName.GetName()], NewMemberName.GetName());
		}

		// Test retrieval via old member name
		TestEqual(TEXT("Should be able to get member value using old name"), System.GetStringValue(FLiveConfigProperty(OldMemberName.GetName(), true)), TEXT("StructRedirectValue"));

		System.PropertyRedirects = OriginalRedirects;
	}

	// Restore original state
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigDoesPropertyNameExistTest, "LiveConfig.System.DoesPropertyNameExist", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigDoesPropertyNameExistTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	FLiveConfigPropertyDefinition Def;
	Def.PropertyName = FLiveConfigProperty(FName(TEXT("Exist.Check.Present")));
	Def.PropertyType = ELiveConfigPropertyType::String;
	Def.Value = TEXT("hello");
	System.PropertyDefinitions.Add(Def.PropertyName, Def);

	TestTrue(TEXT("Existing property should be found"), ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty(TEXT("Exist.Check.Present"))));
	TestFalse(TEXT("Absent property should not be found"), ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty(TEXT("Exist.Check.Absent"))));
	TestFalse(TEXT("NAME_None should not be found"), ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty(NAME_None)));

	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigGetSubPropertiesTest, "LiveConfig.System.GetSubProperties", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigGetSubPropertiesTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	auto AddProp = [&](const TCHAR* Name, ELiveConfigPropertyType Type)
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(Name));
		Def.PropertyType = Type;
		System.PropertyDefinitions.Add(Def.PropertyName, Def);
	};

	AddProp(TEXT("SubProp.Parent.Child1"), ELiveConfigPropertyType::String);
	AddProp(TEXT("SubProp.Parent.Child2"), ELiveConfigPropertyType::Int);
	AddProp(TEXT("SubProp.Parent.Nested.Deep"), ELiveConfigPropertyType::Float);
	AddProp(TEXT("SubProp.Other.Unrelated"), ELiveConfigPropertyType::Bool);
	AddProp(TEXT("SubProp.ParentExtended.NotAMatch"), ELiveConfigPropertyType::String);  // must NOT match SubProp.Parent

	// Without trailing dot
	TArray<FLiveConfigProperty> Result;
	System.GetSubProperties(TEXT("SubProp.Parent"), Result);
	TestEqual(TEXT("Should find exactly 3 sub-properties"), Result.Num(), 3);
	TestTrue(TEXT("Child1 should be found"), Result.ContainsByPredicate([](const FLiveConfigProperty& P){ return P.GetName() == FName(TEXT("SubProp.Parent.Child1")); }));
	TestTrue(TEXT("Child2 should be found"), Result.ContainsByPredicate([](const FLiveConfigProperty& P){ return P.GetName() == FName(TEXT("SubProp.Parent.Child2")); }));
	TestTrue(TEXT("Nested.Deep should be found"), Result.ContainsByPredicate([](const FLiveConfigProperty& P){ return P.GetName() == FName(TEXT("SubProp.Parent.Nested.Deep")); }));
	TestFalse(TEXT("ParentExtended.NotAMatch must NOT be found (dot boundary)"), Result.ContainsByPredicate([](const FLiveConfigProperty& P){ return P.GetName() == FName(TEXT("SubProp.ParentExtended.NotAMatch")); }));

	// With trailing dot - should return same result
	TArray<FLiveConfigProperty> ResultWithDot;
	System.GetSubProperties(TEXT("SubProp.Parent."), ResultWithDot);
	TestEqual(TEXT("Trailing-dot form should return same count"), ResultWithDot.Num(), 3);

	// Empty path guard
	TArray<FLiveConfigProperty> ResultEmpty;
	System.GetSubProperties(NAME_None, ResultEmpty);
	TestEqual(TEXT("Empty path should return nothing"), ResultEmpty.Num(), 0);

	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigRemoveRedirectTest, "LiveConfig.System.RemoveRedirect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigRemoveRedirectTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FName, FName> OriginalRedirects = System.PropertyRedirects;
	System.PropertyRedirects.Empty();

	System.PropertyRedirects.Add(TEXT("Remove.Old"), TEXT("Remove.New"));
	TestTrue(TEXT("Redirect should exist before removal"), System.PropertyRedirects.Contains(TEXT("Remove.Old")));

	System.RemoveRedirect(TEXT("Remove.Old"));
	TestFalse(TEXT("Redirect should be gone after removal"), System.PropertyRedirects.Contains(TEXT("Remove.Old")));

	// Idempotent - calling again on removed key should not crash
	System.RemoveRedirect(TEXT("Remove.Old"));
	TestFalse(TEXT("Second removal should be a no-op"), System.PropertyRedirects.Contains(TEXT("Remove.Old")));

	// Non-existent key
	System.RemoveRedirect(TEXT("Never.Existed"));
	TestTrue(TEXT("Removing non-existent key should not crash"), true);

	System.PropertyRedirects = OriginalRedirects;
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigSavePropertyTest, "LiveConfig.System.SaveProperty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigSavePropertyTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();
	System.RebuildConfigCache();

	FLiveConfigProperty TestProp(FName(TEXT("Test.SaveProp.Alpha")));

	FLiveConfigPropertyDefinition Def;
	Def.PropertyName = TestProp;
	Def.PropertyType = ELiveConfigPropertyType::Int;
	Def.Value = TEXT("77");

	System.SaveProperty(Def);
	JsonSystem->FlushPendingSaves();

	TestTrue(TEXT("SaveProperty should add to definitions"), System.PropertyDefinitions.Contains(TestProp));
	TestEqual(TEXT("Cache should reflect saved value"), System.GetIntValue(TestProp), 77);
	TestTrue(TEXT("Property file should exist on disk"), FPaths::FileExists(JsonSystem->GetPropertyPath(TestProp.GetName())));

	// Update
	Def.Value = TEXT("99");
	System.SaveProperty(Def);
	JsonSystem->FlushPendingSaves();
	TestEqual(TEXT("Cache should reflect updated value"), System.GetIntValue(TestProp), 99);
	TestEqual(TEXT("Definition should reflect updated value"), System.PropertyDefinitions[TestProp].Value, TEXT("99"));

	// Teardown
	JsonSystem->DeletePropertyFile(TestProp.GetName());
	JsonSystem->FlushPendingSaves();
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigDeletePropertyTest, "LiveConfig.System.DeleteProperty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigDeletePropertyTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	FLiveConfigProperty TestProp(FName(TEXT("Test.Delete.Beta")));

	FLiveConfigPropertyDefinition Def;
	Def.PropertyName = TestProp;
	Def.PropertyType = ELiveConfigPropertyType::String;
	Def.Value = TEXT("DeleteMe");
	System.PropertyDefinitions.Add(TestProp, Def);
	System.RebuildConfigCache();

	TestTrue(TEXT("Property should exist before deletion"), System.PropertyDefinitions.Contains(TestProp));
	TestEqual(TEXT("Value should be readable before deletion"), System.GetStringValue(TestProp), TEXT("DeleteMe"));

	System.DeleteProperty(TestProp);

	TestFalse(TEXT("Property should be gone after deletion"), System.PropertyDefinitions.Contains(TestProp));
	TestEqual(TEXT("GetStringValue should return empty after deletion"), System.GetStringValue(TestProp), TEXT(""));

	// Deleting a non-existent property should not crash
	System.DeleteProperty(FLiveConfigProperty(FName(TEXT("Test.Delete.DoesNotExist"))));
	TestTrue(TEXT("Deleting non-existent property should not crash"), true);

	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigProfileOverrideLayeringTest, "LiveConfig.System.ProfileOverrideLayering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigProfileOverrideLayeringTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	FLiveConfigProfile OriginalProfile = ProfileSystem->GetActiveProfile();

	System.PropertyDefinitions.Empty();

	FLiveConfigProperty SpeedProp(FName(TEXT("Test.Profile.Speed")));
	FLiveConfigPropertyDefinition BaseDef;
	BaseDef.PropertyName = SpeedProp;
	BaseDef.PropertyType = ELiveConfigPropertyType::Float;
	BaseDef.Value = TEXT("1.0");
	System.PropertyDefinitions.Add(SpeedProp, BaseDef);
	System.RebuildConfigCache();

	// Base value
	TestNearlyEqual(TEXT("Base value should be 1.0"), System.GetFloatValue(SpeedProp), 1.0f, 0.001f);

	// Apply profile override
	FLiveConfigProfile ProfileA;
	ProfileA.ProfileName = TEXT("TestProfileA");
	ProfileA.Overrides.Add(SpeedProp, TEXT("5.0"));
	ProfileSystem->SetActiveProfileData(ProfileA);

	TestNearlyEqual(TEXT("Profile override should return 5.0"), System.GetFloatValue(SpeedProp), 5.0f, 0.001f);
	TestEqual(TEXT("Active profile name should match"), ProfileSystem->GetActiveProfile().ProfileName, FName(TEXT("TestProfileA")));

	// Clear profile - should revert to base
	ProfileSystem->SetActiveProfileData(FLiveConfigProfile{});
	TestNearlyEqual(TEXT("Clearing profile should revert to base value"), System.GetFloatValue(SpeedProp), 1.0f, 0.001f);

	// Profile with unknown key should not affect other properties
	FLiveConfigProfile ProfileUnknown;
	ProfileUnknown.ProfileName = TEXT("TestProfileUnknown");
	ProfileUnknown.Overrides.Add(FLiveConfigProperty(FName(TEXT("Test.Profile.NonExistent"))), TEXT("999"));
	ProfileSystem->SetActiveProfileData(ProfileUnknown);
	TestNearlyEqual(TEXT("Profile with unknown key should not affect Speed"), System.GetFloatValue(SpeedProp), 1.0f, 0.001f);

	// Delegate fires when profile is applied
	bool bDelegateFired = false;
	FDelegateHandle Handle = ProfileSystem->OnProfileChanged.AddLambda([&bDelegateFired](const FLiveConfigProfile&) { bDelegateFired = true; });
	ProfileSystem->SetActiveProfileData(ProfileA);
	TestTrue(TEXT("OnProfileChanged should fire when profile is applied"), bDelegateFired);
	ProfileSystem->OnProfileChanged.Remove(Handle);

	// Teardown - restore profile first (triggers RebuildConfigCache), then definitions
	ProfileSystem->SetActiveProfileData(OriginalProfile);
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigEnvironmentOverridesTest, "LiveConfig.System.EnvironmentOverrides", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigEnvironmentOverridesTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get();
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();
	// Note: JsonSystem must stay active - PatchEnvironmentOverrides fires OnDataProviderSync which
	// calls SavePropertyDeferred, which calls ULiveConfigJsonSystem::Get()->SavePropertyToFile without a null check.

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	FLiveConfigProfile OriginalProfile = ProfileSystem->GetActiveProfile();
	ProfileSystem->SetActiveProfileData(FLiveConfigProfile{});

	System.PropertyDefinitions.Empty();

	FLiveConfigProperty MultProp(FName(TEXT("Test.Env.Multiplier")));
	FLiveConfigPropertyDefinition BaseDef;
	BaseDef.PropertyName = MultProp;
	BaseDef.PropertyType = ELiveConfigPropertyType::Float;
	BaseDef.Value = TEXT("1.0");
	System.PropertyDefinitions.Add(MultProp, BaseDef);

	// Clear any existing env overrides before starting
	System.PatchEnvironmentOverrides(FLiveConfigProfile{});
	System.RebuildConfigCache();

	TestNearlyEqual(TEXT("Base value should be 1.0 before any env override"), System.GetFloatValue(MultProp), 1.0f, 0.001f);

	int32 DelegateFiredCount = 0;
	FDelegateHandle Handle = System.OnPropertiesUpdated.AddLambda([&DelegateFiredCount]() { DelegateFiredCount++; });

	// Apply env override
	FLiveConfigProfile EnvProfile;
	EnvProfile.Overrides.Add(MultProp, TEXT("3.0"));
	System.PatchEnvironmentOverrides(EnvProfile);
	JsonSystem->FlushPendingSaves();

	TestNearlyEqual(TEXT("Env override should return 3.0"), System.GetFloatValue(MultProp), 3.0f, 0.001f);
	TestTrue(TEXT("OnPropertiesUpdated should fire on first patch"), DelegateFiredCount >= 1);

	// Idempotency - identical profile should not re-fire
	int32 CountBefore = DelegateFiredCount;
	System.PatchEnvironmentOverrides(EnvProfile);
	TestEqual(TEXT("Second patch with same profile should not re-fire delegate"), DelegateFiredCount, CountBefore);

	// Profile layer beats env layer
	FLiveConfigProfile ActiveProfile;
	ActiveProfile.ProfileName = TEXT("TestEnvActiveProfile");
	ActiveProfile.Overrides.Add(MultProp, TEXT("7.0"));
	ProfileSystem->SetActiveProfileData(ActiveProfile);
	TestNearlyEqual(TEXT("Active profile should beat env override"), System.GetFloatValue(MultProp), 7.0f, 0.001f);

	// Clearing active profile exposes env override
	ProfileSystem->SetActiveProfileData(FLiveConfigProfile{});
	TestNearlyEqual(TEXT("Clearing profile should expose env override (not base)"), System.GetFloatValue(MultProp), 3.0f, 0.001f);

	System.OnPropertiesUpdated.Remove(Handle);
	System.PatchEnvironmentOverrides(FLiveConfigProfile{});
	JsonSystem->FlushPendingSaves();

	// Teardown - clean up the file written by SavePropertyDeferred during env sync
	JsonSystem->DeletePropertyFile(MultProp.GetName());
	JsonSystem->FlushPendingSaves();
	ProfileSystem->SetActiveProfileData(OriginalProfile);
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigGetLiveConfigValueRedirectTest, "LiveConfig.System.GetLiveConfigValueRedirect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigGetLiveConfigValueRedirectTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FName, FName> OriginalRedirects = System.PropertyRedirects;
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyRedirects.Empty();
	System.PropertyDefinitions.Empty();

	FLiveConfigProperty NewName(FName(TEXT("Test.Value.NewName")));
	FLiveConfigProperty OldName(FName(TEXT("Test.Value.OldName")));

	FLiveConfigPropertyDefinition Def;
	Def.PropertyName = NewName;
	Def.PropertyType = ELiveConfigPropertyType::String;
	Def.Value = TEXT("RedirectedResult");
	System.PropertyDefinitions.Add(NewName, Def);
	System.PropertyRedirects.Add(OldName.GetName(), NewName.GetName());
	System.RebuildConfigCache();

	// GetStringValue does NOT apply redirects - old name is a cache miss
	TestEqual(TEXT("GetStringValue with old name should be empty (no redirect)"), System.GetStringValue(OldName), TEXT(""));

	// GetLiveConfigValue DOES apply redirects
	TestEqual(TEXT("GetLiveConfigValue with old name should return redirected value"), System.GetLiveConfigValue<FString>(OldName), TEXT("RedirectedResult"));

	// Both should work with the new name directly
	TestEqual(TEXT("GetStringValue with new name should work"), System.GetStringValue(NewName), TEXT("RedirectedResult"));
	TestEqual(TEXT("GetLiveConfigValue with new name should work"), System.GetLiveConfigValue<FString>(NewName), TEXT("RedirectedResult"));

	System.PropertyRedirects = OriginalRedirects;
	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigGetProfileDiffTest, "LiveConfig.System.GetProfileDiff", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigGetProfileDiffTest::RunTest(const FString& Parameters)
{
	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	FSubsystemCollection<UEngineSubsystem>::DeactivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());

	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	FLiveConfigProperty PropA(FName(TEXT("Diff.Prop.A")));
	FLiveConfigProperty PropB(FName(TEXT("Diff.Prop.B")));

	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = PropA;
		Def.PropertyType = ELiveConfigPropertyType::String;
		Def.Value = TEXT("BaseA");
		System.PropertyDefinitions.Add(PropA, Def);
	}
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = PropB;
		Def.PropertyType = ELiveConfigPropertyType::Float;
		Def.Value = TEXT("1.0");
		System.PropertyDefinitions.Add(PropB, Def);
	}
	System.RebuildConfigCache();

	// Profile with one change, one same value, one unknown key
	FLiveConfigProfile Profile;
	Profile.Overrides.Add(PropA, TEXT("ChangedA"));
	Profile.Overrides.Add(PropB, TEXT("1.0"));  // same as base
	Profile.Overrides.Add(FLiveConfigProperty(FName(TEXT("Diff.Prop.Unknown"))), TEXT("x"));

	TArray<FLiveConfigPropertyDefinition> Diff = System.GetProfileDiff(Profile);
	TestEqual(TEXT("Only changed properties should appear in diff"), Diff.Num(), 1);
	TestEqual(TEXT("Diff should contain PropA"), Diff[0].PropertyName.GetName(), PropA.GetName());
	TestEqual(TEXT("Diff should reflect new value"), Diff[0].Value, TEXT("ChangedA"));

	// GetDiffString should format correctly
	FString DiffStr = System.GetDiffString(Diff);
	TestTrue(TEXT("DiffString should contain property name"), DiffStr.Contains(TEXT("Diff.Prop.A")));
	TestTrue(TEXT("DiffString should contain old value"), DiffStr.Contains(TEXT("BaseA")));
	TestTrue(TEXT("DiffString should contain new value"), DiffStr.Contains(TEXT("ChangedA")));
	TestTrue(TEXT("DiffString should contain arrow"), DiffStr.Contains(TEXT("->")));

	// All-matching profile should produce empty diff
	FLiveConfigProfile MatchingProfile;
	MatchingProfile.Overrides.Add(PropA, TEXT("BaseA"));
	MatchingProfile.Overrides.Add(PropB, TEXT("1.0"));
	TArray<FLiveConfigPropertyDefinition> EmptyDiff = System.GetProfileDiff(MatchingProfile);
	TestEqual(TEXT("All-matching profile should produce empty diff"), EmptyDiff.Num(), 0);

	System.PropertyDefinitions = OriginalDefinitions;
	System.RebuildConfigCache();
	FSubsystemCollection<UEngineSubsystem>::ActivateExternalSubsystem(ULiveConfigJsonSystem::StaticClass());
	return true;
}

