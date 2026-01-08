// All rights reserved - Genpop

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LiveConfigCurveTableUpdater.generated.h"

class UCurveTable;

/**
 * Subsystem that handles updating CurveTables with LiveConfig values.
 */
UCLASS()
class LIVECONFIG_API ULiveConfigCurveTableUpdater : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void UpdateCurveTables();
	
protected:
	UFUNCTION()
	void OnPropertiesUpdated();

	void ImportFromCurveTables();
	void ExportToCurveTables();
	void FillCurveTables();

	UPROPERTY()
	UCurveTable* ExportActiveCurveTable;

	UPROPERTY()
	TArray<UCurveTable*> ImportActiveCurveTables;
};
