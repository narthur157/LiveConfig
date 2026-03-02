// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

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
	void ImportFromCurveTables();
	void ExportToCurveTables();
	
	FTimerHandle SaveTimerHandle;

	UPROPERTY()
	TObjectPtr<UCurveTable> ExportActiveCurveTable;
	
	UPROPERTY()
	TArray<TObjectPtr<UCurveTable>> ImportActiveCurveTables;
};
