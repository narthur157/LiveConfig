#include "LiveConfigJson.h"
#include "LiveConfigSystem.h"
#include "Profiles/LiveConfigProfileSystem.h"
#include "Profiles/LiveConfigProfile.h"
#include "LiveConfigGameSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigJsonOperationsTest, "LiveConfig.JsonOperations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigJsonOperationsTest::RunTest(const FString& Parameters)
{
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();
	if (!JsonSystem)
	{
		return false;
	}

	ULiveConfigSystem& System = ULiveConfigSystem::Get();
	
	// Backup original settings
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = System.PropertyDefinitions;
	System.PropertyDefinitions.Empty();

	FName PropertyName = TEXT("Test.Property.Unit");
	FString TestValue = TEXT("UnitTestValue");

	FLiveConfigPropertyDefinition PropertyDefinition;
	PropertyDefinition.PropertyName = FLiveConfigProperty(PropertyName);
	PropertyDefinition.Value = TestValue;
	PropertyDefinition.PropertyType = ELiveConfigPropertyType::String;
	PropertyDefinition.Description = TEXT("Unit Test Description");

	// Test Saving
	JsonSystem->SavePropertyToFile(PropertyDefinition);
	FString ExpectedPath = JsonSystem->GetPropertyPath(PropertyName);
	TestTrue(TEXT("Property file should exist after saving"), FPaths::FileExists(ExpectedPath));

	// Test Content
	FString SavedJson;
	FFileHelper::LoadFileToString(SavedJson, *ExpectedPath);
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SavedJson);
	TestTrue(TEXT("Saved JSON should be valid"), FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid());
	
	FLiveConfigPropertyDefinition LoadedDef;
	FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &LoadedDef);
	TestEqual(TEXT("Loaded property name should match"), LoadedDef.PropertyName.GetName(), PropertyName);
	TestEqual(TEXT("Loaded value should match"), LoadedDef.Value, TestValue);

	// Verify that propertyName is a string, not an object
	FString PropNameValue;
	if (JsonObject->TryGetStringField(TEXT("propertyName"), PropNameValue))
	{
		TestEqual(TEXT("propertyName should be serialized as a string"), PropNameValue, PropertyName.ToString());
	}
	else
	{
		AddError(TEXT("propertyName should be a string field in the JSON"));
	}

	// Test Updating
	PropertyDefinition.Value = TEXT("UpdatedValue");
	JsonSystem->SavePropertyToFile(PropertyDefinition);
	FFileHelper::LoadFileToString(SavedJson, *ExpectedPath);
	Reader = TJsonReaderFactory<>::Create(SavedJson);
	FJsonSerializer::Deserialize(Reader, JsonObject);
	FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &LoadedDef);
	TestEqual(TEXT("Updated value should match"), LoadedDef.Value, TEXT("UpdatedValue"));

	// Test Deleting
	JsonSystem->DeletePropertyFile(PropertyName);
	TestFalse(TEXT("Property file should not exist after deletion"), FPaths::FileExists(ExpectedPath));

	// Test skipping FromCurveTable properties
	FName CurveTableName = TEXT("Test.CurveTable.Property");
	FLiveConfigPropertyDefinition CurveTableDef;
	CurveTableDef.PropertyName = FLiveConfigProperty(CurveTableName);
	CurveTableDef.Value = TEXT("CurveValue");
	CurveTableDef.Tags.Add(LiveConfigTags::FromCurveTable);
	
	JsonSystem->SavePropertyToFile(CurveTableDef);
	FString CurveTablePath = JsonSystem->GetPropertyPath(CurveTableName);
	TestFalse(TEXT("CurveTable property file should NOT exist after saving"), FPaths::FileExists(CurveTablePath));

	// Test Float Precision
	FName FloatPropName = TEXT("Test.Float.Precision");
	FLiveConfigPropertyDefinition FloatDef;
	FloatDef.PropertyName = FLiveConfigProperty(FloatPropName);
	FloatDef.PropertyType = ELiveConfigPropertyType::Float;
	FloatDef.Value = TEXT("0.20000000298023224");

	JsonSystem->SavePropertyToFile(FloatDef);
	FString FloatPath = JsonSystem->GetPropertyPath(FloatPropName);
	TestTrue(TEXT("Float property file should exist"), FPaths::FileExists(FloatPath));

	FString FloatSavedJson;
	FFileHelper::LoadFileToString(FloatSavedJson, *FloatPath);
	TSharedPtr<FJsonObject> FloatJsonObject;
	TSharedRef<TJsonReader<>> FloatReader = TJsonReaderFactory<>::Create(FloatSavedJson);
	FJsonSerializer::Deserialize(FloatReader, FloatJsonObject);
	
	FLiveConfigPropertyDefinition FloatLoadedDef;
	FJsonObjectConverter::JsonObjectToUStruct(FloatJsonObject.ToSharedRef(), &FloatLoadedDef);
	
	// Should be sanitized to "0.2"
	TestEqual(TEXT("Float value should be sanitized to float precision"), FloatLoadedDef.Value, TEXT("0.2"));

	JsonSystem->DeletePropertyFile(FloatPropName);

	// Test invalid names (empty and trailing dot)
	FLiveConfigPropertyDefinition EmptyNameDef;
	EmptyNameDef.PropertyName = FLiveConfigProperty(NAME_None);
	JsonSystem->SavePropertyToFile(EmptyNameDef);
	FString EmptyPath = JsonSystem->GetPropertyPath(NAME_None);
	TestFalse(TEXT("Empty name property file should NOT exist"), FPaths::FileExists(EmptyPath));

	FLiveConfigPropertyDefinition TrailingDotDef;
	TrailingDotDef.PropertyName = FLiveConfigProperty(FName(TEXT("Folder.")));
	JsonSystem->SavePropertyToFile(TrailingDotDef);
	FString TrailingDotPath = JsonSystem->GetPropertyPath(FName(TEXT("Folder.")));
	TestFalse(TEXT("Trailing dot property file should NOT exist"), FPaths::FileExists(TrailingDotPath));

	// Restore original settings
	System.PropertyDefinitions = OriginalDefinitions;

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigProfileJsonTest, "LiveConfig.ProfileJson", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigProfileJsonTest::RunTest(const FString& Parameters)
{
	ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get();
	if (!ProfileSystem)
	{
		return false;
	}

	FName TestProfileName = TEXT("UnitTestProfile");
	FLiveConfigProfile Profile;
	Profile.ProfileName = TestProfileName;
	Profile.Overrides.Add(FLiveConfigProperty(FName(TEXT("Test.Property.1"))), TEXT("Value1"));
	Profile.Overrides.Add(FLiveConfigProperty(FName(TEXT("Test.Property.2"))), TEXT("Value2"));
	Profile.PrepareForReplication(); // Populates ReplicatedOverrides

	// Test Saving
	ProfileSystem->SaveProfile(Profile);
	FString Path = ProfileSystem->GetProfilePath(TestProfileName);
	TestTrue(TEXT("Profile file should exist"), FPaths::FileExists(Path));

	FString SavedJson;
	FFileHelper::LoadFileToString(SavedJson, *Path);
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SavedJson);
	TestTrue(TEXT("Saved JSON should be valid"), FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid());

	// Verify that profileName is NOT present
	TestFalse(TEXT("profileName should NOT be serialized in the new format"), JsonObject->HasField(TEXT("profileName")));

	// Verify that overrides is NOT a nested field, but the root object itself contains the properties
	// In the new format, the root JSON object IS the overrides map
	FString Val1;
	if (JsonObject->TryGetStringField(TEXT("Test.Property.1"), Val1))
	{
		TestEqual(TEXT("Override value 1 should match"), Val1, TEXT("Value1"));
	}
	else
	{
		AddError(TEXT("Override Test.Property.1 should be a string field at the root (flattened)"));
	}

	// Test Round-trip
	FLiveConfigProfile LoadedProfile;
	TestTrue(TEXT("LoadProfile should succeed"), ProfileSystem->LoadProfile(TestProfileName, LoadedProfile));
	TestEqual(TEXT("Loaded profile name should match"), LoadedProfile.ProfileName, TestProfileName);
	TestEqual(TEXT("Loaded overrides count should match"), LoadedProfile.Overrides.Num(), 2);
	TestEqual(TEXT("Loaded override value 1 should match"), LoadedProfile.Overrides.FindRef(FLiveConfigProperty(FName(TEXT("Test.Property.1")))), TEXT("Value1"));

	// Cleanup
	ProfileSystem->DeleteProfile(TestProfileName);

	return true;
}
