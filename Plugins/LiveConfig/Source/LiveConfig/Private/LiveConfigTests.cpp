#include "LiveConfigJson.h"
#include "LiveConfigGameSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigJsonOperationsTest, "LiveConfig.JsonOperations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLiveConfigJsonOperationsTest::RunTest(const FString& Parameters)
{
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();
	if (!JsonSystem)
	{
		return false;
	}

	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	
	// Backup original settings
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = Settings->PropertyDefinitions;
	Settings->PropertyDefinitions.Empty();

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

	// Restore original settings
	Settings->PropertyDefinitions = OriginalDefinitions;

	return true;
}
