#include "LiveConfigGameSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ComparisonUtility.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigNaturalSortTest, "LiveConfig.NaturalSort", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigNaturalSortTest::RunTest(const FString& Parameters)
{
    TArray<FString> PropertyNames;
    PropertyNames.Add(TEXT("Property.10"));
    PropertyNames.Add(TEXT("Property.2"));
    PropertyNames.Add(TEXT("Property.1"));
    PropertyNames.Add(TEXT("Property.20"));
    PropertyNames.Add(TEXT("Property.A"));
    PropertyNames.Add(TEXT("Property.B"));

    PropertyNames.Sort([](const FString& A, const FString& B)
    {
        return UE::ComparisonUtility::CompareNaturalOrder(A, B) < 0;
    });

    TestEqual(TEXT("First item should be Property.1"), PropertyNames[0], TEXT("Property.1"));
    TestEqual(TEXT("Second item should be Property.2"), PropertyNames[1], TEXT("Property.2"));
    TestEqual(TEXT("Third item should be Property.10"), PropertyNames[2], TEXT("Property.10"));
    TestEqual(TEXT("Fourth item should be Property.20"), PropertyNames[3], TEXT("Property.20"));
    TestEqual(TEXT("Fifth item should be Property.A"), PropertyNames[4], TEXT("Property.A"));
    TestEqual(TEXT("Sixth item should be Property.B"), PropertyNames[5], TEXT("Property.B"));

    return true;
}
