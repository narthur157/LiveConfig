#include "LiveConfigCurveTableUpdater.h"
#include "LiveConfigSystem.h"
#include "LiveConfigTypes.h"
#include "LiveConfigGameSettings.h"
#include "Engine/CurveTable.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveConfigCurveTableTest, "LiveConfig.CurveTable.Export", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FLiveConfigCurveTableTest::RunTest(const FString& Parameters)
{
	ULiveConfigCurveTableUpdater* Updater = NewObject<ULiveConfigCurveTableUpdater>();
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();

	// Backup original settings
	TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition> OriginalDefinitions = Settings->PropertyDefinitions;
	FSoftObjectPath OriginalExportTable = Settings->ExportCurveTable;
	bool bOriginalAutoCreate = Settings->bAutoCreateRowsInExportTable;

	Settings->PropertyDefinitions.Empty();
	Settings->bAutoCreateRowsInExportTable = true;

	// Create a temporary CurveTable for testing
	UCurveTable* TestCurveTable = NewObject<UCurveTable>(GetTransientPackage(), NAME_None, RF_Transient);	
	struct FPublicCurveTableUpdater : public ULiveConfigCurveTableUpdater
	{
		using ULiveConfigCurveTableUpdater::ExportActiveCurveTable;
		using ULiveConfigCurveTableUpdater::ExportToCurveTables;
	};

	FPublicCurveTableUpdater* PublicUpdater = static_cast<FPublicCurveTableUpdater*>(Updater);
	PublicUpdater->ExportActiveCurveTable = TestCurveTable;

	// Test Simple Property Export
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("TestFloat")));
		Def.Value = TEXT("123.45");
		Def.PropertyType = ELiveConfigPropertyType::Float;
		Settings->PropertyDefinitions.Add(Def.PropertyName, Def);
	}

	AddInfo(TEXT("Starting FillCurveTables"));
	PublicUpdater->ExportToCurveTables();
	AddInfo(TEXT("Finished FillCurveTables"));

	if (TestCurveTable->GetSimpleCurveRowMap().Num() == 0)
	{
		AddError(TEXT("Curve table should have at least one row"));
		return false;
	}

	FSimpleCurve* FoundCurve = TestCurveTable->GetSimpleCurveRowMap().FindRef(TEXT("TestFloat"));
	if (!FoundCurve)
	{
		AddError(TEXT("Curve row 'TestFloat' should be created"));
		return false;
	}

	if (FoundCurve->Keys.Num() != 2)
	{
		AddError(FString::Printf(TEXT("Curve should have 2 keys (for 0 and 1), but has %d"), FoundCurve->Keys.Num()));
		return false;
	}

	// 2. Test Indexed Property (Array syntax)
	{
		FLiveConfigPropertyDefinition Def;
		Def.PropertyName = FLiveConfigProperty(FName(TEXT("TestArray.0")));
		Def.Value = TEXT("10");
		Def.PropertyType = ELiveConfigPropertyType::Int;
		Settings->PropertyDefinitions.Add(Def.PropertyName, Def);

		FLiveConfigPropertyDefinition Def1;
		Def1.PropertyName = FLiveConfigProperty(FName(TEXT("TestArray.1")));
		Def1.Value = TEXT("20");
		Def1.PropertyType = ELiveConfigPropertyType::Int;
		Settings->PropertyDefinitions.Add(Def1.PropertyName, Def1);
	}

	PublicUpdater->ExportToCurveTables();

	FSimpleCurve* ArrayCurve = TestCurveTable->GetSimpleCurveRowMap().FindRef(TEXT("TestArray"));
	if (!ArrayCurve)
	{
		AddError(TEXT("Curve row 'TestArray' should be created"));
		return false;
	}

	if (ArrayCurve->Keys.Num() != 2)
	{
		AddError(FString::Printf(TEXT("Array curve should have 2 keys, but has %d"), ArrayCurve->Keys.Num()));
		return false;
	}

	// 3. Test skipping FromCurveTable properties
	{
		FLiveConfigPropertyDefinition CurveDef;
		CurveDef.PropertyName = FLiveConfigProperty(FName(TEXT("InboundProp")));
		CurveDef.Value = TEXT("999");
		CurveDef.PropertyType = ELiveConfigPropertyType::Float;
		CurveDef.Tags.Add(LiveConfigTags::FromCurveTable);
		Settings->PropertyDefinitions.Add(CurveDef.PropertyName, CurveDef);
	}

	PublicUpdater->ExportToCurveTables();
	if (TestCurveTable->GetSimpleCurveRowMap().Contains(TEXT("InboundProp")))
	{
		AddError(TEXT("InboundProp should NOT be exported"));
		return false;
	}

	// Restore original settings
	Settings->PropertyDefinitions = OriginalDefinitions;
	Settings->ExportCurveTable = OriginalExportTable;
	Settings->bAutoCreateRowsInExportTable = bOriginalAutoCreate;

	return true;
}
