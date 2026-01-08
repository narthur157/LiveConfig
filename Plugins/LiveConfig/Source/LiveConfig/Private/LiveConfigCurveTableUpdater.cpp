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

	static const FName FromCurveTableTag = TEXT("FromCurveTable");

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
					
					if (Def.PropertyName != PropName || Def.PropertyType != ELiveConfigPropertyType::Float || Def.Value != FString::SanitizeFloat(Key.Value))
					{
						bSettingsChanged = true;
					}

					Def.PropertyName = PropName;
					Def.PropertyType = ELiveConfigPropertyType::Float;
					Def.Value = FString::SanitizeFloat(Key.Value);
					
					Def.Tags.AddUnique(FromCurveTableTag);
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
	static const FName FromCurveTableTag = TEXT("FromCurveTable");

	if (!ExportActiveCurveTable)
	{
		return;
	}

	// 1. Update existing rows in the Export Table
	for (const TTuple<FName, FSimpleCurve*>& Pair : ExportActiveCurveTable->GetSimpleCurveRowMap())
	{
		FName RowName = Pair.Key;
		FSimpleCurve* Curve = Pair.Value;

		// Exact match
		if (const FLiveConfigPropertyDefinition* Def = Settings->PropertyDefinitions.Find(RowName))
		{
			// Skip properties imported from curve tables
			if (Def->Tags.Contains(FromCurveTableTag))
			{
				continue;
			}

			if (Def->PropertyType == ELiveConfigPropertyType::Float)
			{
				float Value = FCString::Atof(*Def->Value);
				if (Curve->Keys.Num() > 0)
				{
					for (FSimpleCurveKey& Key : Curve->Keys)
					{
						Key.Value = Value;
					}
				}
				else
				{
					Curve->AddKey(0.f, Value);
				}
			}
		}

		// Handle implicit array syntax PropertyName.0, PropertyName.1 etc.
		for (FSimpleCurveKey& Key : Curve->Keys)
		{
			FName ConfigKey = FName(FString::Printf(TEXT("%s.%d"), *RowName.ToString(), FMath::RoundToInt(Key.Time)));
			if (const FLiveConfigPropertyDefinition* Def = Settings->PropertyDefinitions.Find(ConfigKey))
			{
				if (Def->Tags.Contains(FromCurveTableTag))
				{
					continue;
				}

				if (Def->PropertyType == ELiveConfigPropertyType::Float)
				{
					Key.Value = FCString::Atof(*Def->Value);
				}
			}
		}
	}

	// 2. Add/Update rows for all float properties in PropertyDefinitions
	for (const auto& Pair : Settings->PropertyDefinitions)
	{
		const FLiveConfigPropertyDefinition& Def = Pair.Value;
		
		// Skip properties imported from curve tables
		if (Def.Tags.Contains(FromCurveTableTag))
		{
			continue;
		}

		if (Def.PropertyType == ELiveConfigPropertyType::Float)
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
			float Value = FCString::Atof(*Def.Value);
			
			FSimpleCurve* Curve = ExportActiveCurveTable->GetSimpleCurveRowMap().FindRef(RowName);
			if (!Curve)
			{
				if (Settings->bAutoCreateRowsInExportTable)
				{
					Curve = new FSimpleCurve();
					// Create a mutable copy of the map, add the item, and then assign it back if possible.
					// However, GetSimpleCurveRowMap() in UCurveTable usually returns a reference to the internal map.
					// The error "function is missing const qualifier" suggests GetSimpleCurveRowMap() might be returning a const reference.
					// Let's check how to get a mutable map or use a different approach.
					// In UE, UCurveTable::GetSimpleCurveRowMap() returns TMap<FName, FSimpleCurve*>&.
					
					// Let's try casting away const if it's indeed returning const for some reason, 
					// but wait, I am calling it on a non-const pointer ExportActiveCurveTable.
					
					// Re-reading the error: "function is missing const qualifier" on TMapBase::Add.
					// This is strange. Usually it means the TMap itself is const.
					
					// Let's try this:
					TMap<FName, FSimpleCurve*>& RowMap = const_cast<TMap<FName, FSimpleCurve*>&>(ExportActiveCurveTable->GetSimpleCurveRowMap());
					RowMap.Add(RowName, Curve);
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
						Key.Value = Value;
						bKeyUpdated = true;
						break;
					}
				}
				if (!bKeyUpdated)
				{
					Curve->AddKey(static_cast<float>(Index), Value);
				}
			}
			else
			{
				// Single value, usually means it should be at all keys or at least at 0 and 1
				if (Curve->Keys.Num() == 0)
				{
					Curve->AddKey(0.f, Value);
					Curve->AddKey(1.f, Value);
				}
				else
				{
					for (FSimpleCurveKey& Key : Curve->Keys)
					{
						Key.Value = Value;
					}
				}
			}
		}
	}

	ExportActiveCurveTable->Modify();
#if WITH_EDITOR
	ExportActiveCurveTable->PostEditChange();

	// Automatically save the asset in the editor
	UPackage* Package = ExportActiveCurveTable->GetOutermost();
	if (Package)
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
