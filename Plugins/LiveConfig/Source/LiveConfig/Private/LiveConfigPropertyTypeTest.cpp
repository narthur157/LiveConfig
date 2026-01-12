#include "LiveConfigPropertyName.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigPropertyTest, "LiveConfig.Property", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigPropertyTest::RunTest(const FString& Parameters)
{
	// Test Constructors
	{
		FLiveConfigProperty PropNone;
		TestEqual(TEXT("Default constructor should be NAME_None"), PropNone.GetName(), NAME_None);
		TestFalse(TEXT("Default constructor should be invalid"), PropNone.IsValid());

		FLiveConfigProperty PropName(FName(TEXT("TestName")));
		TestEqual(TEXT("FName constructor should match"), PropName.GetName(), FName(TEXT("TestName")));
		TestTrue(TEXT("FName constructor should be valid"), PropName.IsValid());

		FLiveConfigProperty PropStr(FString(TEXT("TestStr")));
		TestEqual(TEXT("FString constructor should match"), PropStr.GetName(), FName(TEXT("TestStr")));
		TestTrue(TEXT("FString constructor should be valid"), PropStr.IsValid());
	}

	// Test ToString
	{
		FLiveConfigProperty Prop(FName(TEXT("My.Awesome.Property")));
		TestEqual(TEXT("ToString should match"), Prop.ToString(), TEXT("My.Awesome.Property"));
	}

	// Test Equality Operators
	{
		FLiveConfigProperty PropA(FName(TEXT("Same")));
		FLiveConfigProperty PropB(FName(TEXT("Same")));
		FLiveConfigProperty PropC(FName(TEXT("Different")));

		TestTrue(TEXT("PropA should equal PropB"), PropA == PropB);
		TestFalse(TEXT("PropA should not equal PropC"), PropA == PropC);
		TestTrue(TEXT("PropA should not equal PropC (operator!=)"), PropA != PropC);
	}

	// Test Hash (implicitly via TMap or explicitly)
	{
		FLiveConfigProperty Prop(FName(TEXT("HashMe")));
		uint32 Hash = GetTypeHash(Prop);
		TestEqual(TEXT("Hash should match underlying FName hash"), Hash, GetTypeHash(FName(TEXT("HashMe"))));
	}

	return true;
}
