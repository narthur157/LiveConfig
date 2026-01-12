// All rights reserved - Genpop

#include "LiveConfigCurveTableUpdater.h"
#include "LiveConfigSystem.h"
#include "LiveConfigGameSettings.h"
#include "Engine/CurveTable.h"

#if WITH_EDITOR
#include "Editor.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

void ULiveConfigCurveTableUpdater::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency(ULiveConfigSystem::StaticClass());

	UpdateCurveTables();
	
	ULiveConfigSystem* System = ULiveConfigSystem::Get();
	if (System)
	{
		System->OnPropertiesUpdated.AddUObject(this, &ThisClass::OnPropertiesUpdated);
	}
}

void ULiveConfigCurveTableUpdater::UpdateCurveTables()
{
	ExportActiveCurveTable = nullptr;
	ImportActiveCurveTables.Empty();
	
	const ULiveConfigGameSettings* Settings = GetDefault<ULiveConfigGameSettings>();
	
	// Handle Export
	if (Settings->ExportCurveTable.IsValid())
	{
		ExportActiveCurveTable = Cast<UCurveTable>(Settings->ExportCurveTable.TryLoad());
	}

	// Handle Imports
	for (const FSoftObjectPath& Path : Settings->ImportCurveTables)
	{
		if (UCurveTable* CurveTable = Cast<UCurveTable>(Path.TryLoad()))
		{
			ImportActiveCurveTables.Add(CurveTable);
		}
	}

	ImportFromCurveTables();
	ExportToCurveTables();
}

void ULiveConfigCurveTableUpdater::OnPropertiesUpdated()
{
	ExportToCurveTables();
}

void ULiveConfigCurveTableUpdater::ImportFromCurveTables()
{
	ULiveConfigGameSettings* Settings = GetMutableDefault<ULiveConfigGameSettings>();
	if (!Settings)
	{
		return;
	}

	for (UCurveTable* CurveTable : ImportActiveCurveTables)
	{
		FName TableTag = CurveTable->GetFName();

		bool bSettingsChanged = false;

		// Add the table tag to known tags if it's missing
		if (!Settings->KnownTags.Contains(TableTag))
		{
			Settings->KnownTags.Add(TableTag);
			bSettingsChanged = true;
		}

		for (const TTuple<FName, FSimpleCurve*>& Pair : CurveTable->GetSimpleCurveRowMap())
		{
			FName RowName = Pair.Key;
			FSimpleCurve* Curve = Pair.Value;

			if (Curve->Keys.Num() > 0)
			{
				for (const FSimpleCurveKey& Key : Curve->Keys)
				{
					FName PropName = (Curve->Keys.Num() == 1) ? RowName : FName(FString::Printf(TEXT("%s.%d"), *RowName.ToString(), FMath::RoundToInt(Key.Time)));
					
					FLiveConfigPropertyDefinition& Def = Settings->PropertyDefinitions.FindOrAdd(PropName);
					
					if (Def.PropertyName != PropName || (Def.PropertyType != ELiveConfigPropertyType::Float && Def.PropertyType != ELiveConfigPropertyType::Int) || Def.Value != FString::SanitizeFloat(Key.Value))
					{
						bSettingsChanged = true;
					}

					Def.PropertyName = PropName;
					if (Def.PropertyType != ELiveConfigPropertyType::Int)
					{
						Def.PropertyType = ELiveConfigPropertyType::Float;
					}
					Def.Value = FString::SanitizeFloat(Key.Value);
					
					Def.Tags.AddUnique(LiveConfigTags::FromCurveTable);
					Def.Tags.AddUnique(TableTag);

					if (Def.Description.IsEmpty())
					{
						Def.Description = FString::Printf(TEXT("Imported from %s"), *CurveTable->GetName());
					}
				}
			}
		}

		if (bSettingsChanged)
		{
			Settings->Modify();
			Settings->SaveConfig();
		}
	}
}

void ULiveConfigCurveTableUpdater::ExportToCurveTables()
{
	FillCurveTables();
}

void ULiveConfigCurveTableUpdater::FillCurveTables()
{
	const ULiveConfigGameSettings* Settings = GetDefault<ULiveConfigGameSettings>();

	if (!ExportActiveCurveTable)
	{
		return;
	}

	bool bAnythingChanged = false;

	// 1. Update existing rows in the Export Table
	for (const TTuple<FName, FSimpleCurve*>& Pair : ExportActiveCurveTable->GetSimpleCurveRowMap())
	{
		FName RowName = Pair.Key;
		FSimpleCurve* Curve = Pair.Value;

		// Exact match
		if (const FLiveConfigPropertyDefinition* Def = Settings->PropertyDefinitions.Find(RowName))
		{
			// Skip properties imported from curve tables
			if (Def->Tags.Contains(LiveConfigTags::FromCurveTable))
			{
				continue;
			}

			if (Def->PropertyType == ELiveConfigPropertyType::Float || Def->PropertyType == ELiveConfigPropertyType::Int)
			{
				float Value = Def->PropertyType == ELiveConfigPropertyType::Float ? FCString::Atof(*Def->Value) : static_cast<float>(FCString::Atoi(*Def->Value));
				if (Curve->Keys.Num() > 0)
				{
					for (FSimpleCurveKey& Key : Curve->Keys)
					{
						if (!FMath::IsNearlyEqual(Key.Value, Value))
						{
							Key.Value = Value;
							bAnythingChanged = true;
						}
					}
				}
				else
				{
					Curve->AddKey(0.f, Value);
					bAnythingChanged = true;
				}
			}
		}

		// Handle implicit array syntax PropertyName.0, PropertyName.1 etc.
		for (FSimpleCurveKey& Key : Curve->Keys)
		{
			FName ConfigKey = FName(FString::Printf(TEXT("%s.%d"), *RowName.ToString(), FMath::RoundToInt(Key.Time)));
			if (const FLiveConfigPropertyDefinition* Def = Settings->PropertyDefinitions.Find(ConfigKey))
			{
				if (Def->Tags.Contains(LiveConfigTags::FromCurveTable))
				{
					continue;
				}

				if (Def->PropertyType == ELiveConfigPropertyType::Float || Def->PropertyType == ELiveConfigPropertyType::Int)
				{
					float Value = Def->PropertyType == ELiveConfigPropertyType::Float ? FCString::Atof(*Def->Value) : static_cast<float>(FCString::Atoi(*Def->Value));
					if (!FMath::IsNearlyEqual(Key.Value, Value))
					{
						Key.Value = Value;
						bAnythingChanged = true;
					}
				}
			}
		}
	}

	// 2. Add/Update rows for all float and int properties in PropertyDefinitions
	for (const auto& Pair : Settings->PropertyDefinitions)
	{
		const FLiveConfigPropertyDefinition& Def = Pair.Value;
		
		// Skip properties imported from curve tables
		if (Def.Tags.Contains(LiveConfigTags::FromCurveTable))
		{
			continue;
		}

		if (Def.PropertyType == ELiveConfigPropertyType::Float || Def.PropertyType == ELiveConfigPropertyType::Int)
		{
			FName FullName = Def.PropertyName.GetName();
			
			// Split into BaseName and Index if possible (e.g. MyProp.0 -> MyProp, 0)
			FString BaseNameStr;
			FString IndexStr;
			int32 Index = 0;
			bool bHasIndex = false;
			
			FString FullNameStr = FullName.ToString();
			if (FullNameStr.Split(TEXT("."), &BaseNameStr, &IndexStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
			{
				if (IndexStr.IsNumeric())
				{
					Index = FCString::Atoi(*IndexStr);
					bHasIndex = true;
				}
			}
			
			FName RowName = bHasIndex ? FName(*BaseNameStr) : FullName;
			float Value = Def.PropertyType == ELiveConfigPropertyType::Float ? FCString::Atof(*Def.Value) : static_cast<float>(FCString::Atoi(*Def.Value));
			
			FSimpleCurve* Curve = ExportActiveCurveTable->GetSimpleCurveRowMap().FindRef(RowName);
			if (!Curve)
			{
				if (Settings->bAutoCreateRowsInExportTable)
				{
					Curve = &ExportActiveCurveTable->AddSimpleCurve(RowName);
					bAnythingChanged = true;
				}
				else
				{
					continue;
				}
			}
			
			if (bHasIndex)
			{
				// Update or add key at time = Index
				bool bKeyUpdated = false;
				for (FSimpleCurveKey& Key : Curve->Keys)
				{
					if (FMath::IsNearlyEqual(Key.Time, static_cast<float>(Index)))
					{
						if (!FMath::IsNearlyEqual(Key.Value, Value))
						{
							Key.Value = Value;
							bAnythingChanged = true;
						}
						bKeyUpdated = true;
						break;
					}
				}
				if (!bKeyUpdated)
				{
					Curve->AddKey(static_cast<float>(Index), Value);
					bAnythingChanged = true;
				}
			}
			else
			{
				// Single value, usually means it should be at all keys or at least at 0 and 1
				if (Curve->Keys.Num() == 0)
				{
					Curve->AddKey(0.f, Value);
					Curve->AddKey(1.f, Value);
					bAnythingChanged = true;
				}
				else
				{
					for (FSimpleCurveKey& Key : Curve->Keys)
					{
						if (!FMath::IsNearlyEqual(Key.Value, Value))
						{
							Key.Value = Value;
							bAnythingChanged = true;
						}
					}
				}
			}
		}
	}

	if (!bAnythingChanged)
	{
		return;
	}

	ExportActiveCurveTable->Modify();
#if WITH_EDITOR
	ExportActiveCurveTable->PostEditChange();

	// Automatically save the asset in the editor
	UPackage* Package = ExportActiveCurveTable->GetOutermost();
	if (Package && !Package->HasAnyFlags(RF_Transient) && !Package->GetName().StartsWith(TEXT("/Temp/")))
	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.Error = GError;
		SaveArgs.bForceByteSwapping = false;
		SaveArgs.bWarnOfLongFilename = true;
		SaveArgs.SaveFlags = SAVE_NoError;

		UPackage::SavePackage(Package, ExportActiveCurveTable, *PackageFileName, SaveArgs);
	}
#endif
}
