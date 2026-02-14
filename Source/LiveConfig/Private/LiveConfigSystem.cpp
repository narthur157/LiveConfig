#include "LiveConfigSystem.h"

#include "Profiles/LiveConfigProfileSystem.h"
#include "HttpModule.h"
#include "LiveConfigSettings.h"
#include "Interfaces/IHttpResponse.h"
#include "LiveConfigTypes.h"
#include "Serialization/Csv/CsvParser.h"

#include "LiveConfigJson.h"
#include "ConsoleSettings.h"
#include "LiveConfigLib.h"
#include "Engine/Console.h"
#include "Misc/StringOutputDevice.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Exporters/Exporter.h"
#include "UnrealExporter.h"
#endif

namespace LiveConfigTags
{
	const FName FromCurveTable = TEXT("FromCurveTable");
}

ULiveConfigSystem& ULiveConfigSystem::Get()
{
    ULiveConfigSystem* Subsystem = GEngine->GetEngineSubsystem<ULiveConfigSystem>();
    check(Subsystem);
    return *Subsystem;
}

bool ULiveConfigSystem::DoesPropertyNameExist(FLiveConfigProperty PropertyName)
{
    return Get().PropertyDefinitions.Contains(PropertyName);
}

void ULiveConfigSystem::RedirectPropertyName(FLiveConfigProperty& Property) const
{
    FName CurrentName = Property.GetName();
    int32 RedirectCount = 0;
    const int32 MaxRedirects = 10;
    
    while (const FName* RedirectedName = PropertyRedirects.Find(CurrentName))
    {
        CurrentName = *RedirectedName;
        RedirectCount++;
        if (RedirectCount > MaxRedirects)
        {
            UE_LOG(LogLiveConfig, Error, TEXT("Circular redirect detected for property: %s"), *Property.ToString());
            break;
        }
    }
    
    if (CurrentName != Property.GetName())
    {
    	UE_LOG(LogLiveConfig, Verbose, TEXT("Redirected property name: %s -> %s"), *Property.GetName().ToString(), *CurrentName.ToString());
        Property = FLiveConfigProperty(CurrentName);
    }
}

void ULiveConfigSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency(ULiveConfigProfileSystem::StaticClass());
    Collection.InitializeDependency(ULiveConfigJsonSystem::StaticClass());
    
    SheetUrl = GetDefault<ULiveConfigSettings>()->SheetUrl;

    UConsole::RegisterConsoleAutoCompleteEntries.AddUObject(this, &ThisClass::PopulateAutoCompleteEntries);

    if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
    {
        JsonSystem->LoadJsonFromFiles();
    }

    RebuildConfigCache();

   	// Register "FromCurveTable" as a known tag if it's not already there
	PropertyTags.AddUnique(LiveConfigTags::FromCurveTable);

    DownloadConfig();

    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnTravel);
    FWorldDelegates::OnStartGameInstance.AddUObject(this, &ThisClass::OnStartGameInstance);

    if (ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get())
    {
        ProfileSystem->OnProfileChanged.AddUObject(this, &ThisClass::RebuildConfigCache);
    }
}

void ULiveConfigSystem::RebuildConfigCache()
{
    BuildCache();
    OnPropertiesUpdated.Broadcast();
}

void ULiveConfigSystem::RebuildConfigCache(const FLiveConfigProfile& Profile)
{
    RebuildConfigCache();
}

void ULiveConfigSystem::Deinitialize()
{
    Super::Deinitialize();
}

bool ULiveConfigSystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void ULiveConfigSystem::SaveProperty(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
	ULiveConfigJsonSystem::Get()->SavePropertyToFile(PropertyDefinition);
	RebuildConfigCache();
}

void ULiveConfigSystem::SavePropertyDeferred(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
	ULiveConfigJsonSystem::Get()->SavePropertyToFile(PropertyDefinition);
}

void ULiveConfigSystem::RenameProperty(FLiveConfigProperty OldName, FLiveConfigProperty NewName, bool bCreateRedirector)
{
	if (OldName == NewName || !OldName.IsValid() || !NewName.IsValid())
	{
		return;
	}

	if (!PropertyDefinitions.Contains(OldName))
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("Failed to rename property: %s does not exist"), *OldName.ToString());
		return;
	}

	// If it's a struct, we need to rename all its members too
	FLiveConfigPropertyDefinition OldDef = PropertyDefinitions[OldName];
	if (OldDef.PropertyType == ELiveConfigPropertyType::Struct)
	{
		FString OldPrefix = OldName.ToString() + TEXT(".");
		FString NewPrefix = NewName.ToString() + TEXT(".");

		TArray<FLiveConfigProperty> MembersToRename;
		for (const auto& Pair : PropertyDefinitions)
		{
			if (Pair.Key.ToString().StartsWith(OldPrefix))
			{
				MembersToRename.Add(Pair.Key);
			}
		}

		for (const FLiveConfigProperty& OldMemberName : MembersToRename)
		{
			FString RelativeName = OldMemberName.ToString().RightChop(OldPrefix.Len());
			FLiveConfigProperty NewMemberName(NewPrefix + RelativeName);
			RenameProperty(OldMemberName, NewMemberName, bCreateRedirector);
		}
	}

	// Rename the property itself
	FLiveConfigPropertyDefinition NewDef = OldDef;
	NewDef.PropertyName = NewName;

	PropertyDefinitions.Remove(OldName);
	PropertyDefinitions.Add(NewName, NewDef);

	if (bCreateRedirector)
	{
		PropertyRedirects.Add(OldName.GetName(), NewName.GetName());
		TryUpdateDefaultConfigFile();
	}

	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		// Update disk
		JsonSystem->RenamePropertyOnDisk(OldName.GetName(), NewName.GetName());
		JsonSystem->SavePropertyToFile(NewDef);
	}

	RebuildConfigCache();
}

void ULiveConfigSystem::GetUnusedProperties(TArray<FName>& OutUnusedProperties) const
{
	OutUnusedProperties.Empty();

#if WITH_EDITOR
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Get all config values to check for property names in configs
	TArray<FString> AllConfigValues;
	{
		TArray<FString> AllConfigFiles;
		GConfig->GetConfigFilenames(AllConfigFiles);

		for (const FString& ConfigFile : AllConfigFiles)
		{
			if (const FConfigFile* File = GConfig->FindConfigFile(*ConfigFile))
			{
				for (const auto& SectionIt : *File)
				{
					for (const auto& PropertyIt : SectionIt.Value)
					{
						const FString& ConfigValue = PropertyIt.Value.GetValue();
						if (!ConfigValue.IsNumeric())
						{
							AllConfigValues.Add(ConfigValue);
						}
					}
				}
			}
		}
	}

	// Build a reverse map of property redirectors so we can find the old names of a given property to see if any of them are still referenced
	TMultiMap<FName, FName> ReverseRedirectorMap;
	for (const auto& RedirectPair : PropertyRedirects)
	{
		ReverseRedirectorMap.Add(RedirectPair.Value, RedirectPair.Key);
	}

	// Function to determine if a property name is used in assets or configs
	auto IsPropertyNameUsed = [this, &AssetRegistry, &AllConfigValues](FName PropertyName) -> bool
	{
		TArray<FName> NamesToCheck;
		GetRelatedPropertyNames(PropertyName, NamesToCheck);

		for (const FName& NameToCheck : NamesToCheck)
		{
			// Asset check
			FAssetIdentifier PropertyId = FAssetIdentifier(FLiveConfigProperty::StaticStruct(), NameToCheck);
			TArray<FAssetIdentifier> Referencers;
			AssetRegistry.GetReferencers(PropertyId, Referencers, UE::AssetRegistry::EDependencyCategory::SearchableName);

			if (Referencers.Num() > 0)
			{
				return true;
			}

			// Config check
			FString PropertyString = NameToCheck.ToString();
			for (const FString& ConfigValue : AllConfigValues)
			{
				if (ConfigValue.Contains(PropertyString))
				{
					return true;
				}
			}
		}
		return false;
	};

	for (const auto& Pair : PropertyDefinitions)
	{
		const FName PropertyName = Pair.Key.GetName();
		if (!IsPropertyNameUsed(PropertyName))
		{
			OutUnusedProperties.Add(PropertyName);
		}
	}
#endif
}

void ULiveConfigSystem::GetUnusedRedirects(TArray<FName>& OutUnusedRedirects) const
{
	OutUnusedRedirects.Empty();

#if WITH_EDITOR
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Get all config values to check for property names in configs
	TArray<FString> AllConfigValues;
	{
		TArray<FString> AllConfigFiles;
		GConfig->GetConfigFilenames(AllConfigFiles);

		for (const FString& ConfigFile : AllConfigFiles)
		{
			if (const FConfigFile* File = GConfig->FindConfigFile(*ConfigFile))
			{
				for (const auto& SectionIt : *File)
				{
					for (const auto& PropertyIt : SectionIt.Value)
					{
						const FString& ConfigValue = PropertyIt.Value.GetValue();
						if (!ConfigValue.IsNumeric())
						{
							AllConfigValues.Add(ConfigValue);
						}
					}
				}
			}
		}
	}
#endif

	for (const auto& RedirectPair : PropertyRedirects)
	{
		const FName& OldName = RedirectPair.Key;
		const FName& NewName = RedirectPair.Value;

		// Check if the target property exists
		FLiveConfigProperty TargetProperty(NewName);
		if (!PropertyDefinitions.Contains(TargetProperty))
		{
#if WITH_EDITOR
			// Even if target is missing, if the OLD name is still used, we might want to keep the redirect?
			// Actually, if target is missing, the redirect is broken anyway. 
			// But Gameplay Tags check if the OLD name is used.
			
			// For redirects, "Unused" usually means the target is gone.
			// However, if the old name is NOT referenced anywhere and the target is gone, it's definitely safe to remove.
			// If the old name IS referenced but target is gone, it's a broken reference.

			bool bOldNameUsed = false;
			{
				TArray<FName> RelatedNames;
				GetRelatedPropertyNames(OldName, RelatedNames);

				for (const FName& RelatedName : RelatedNames)
				{
					// Asset check
					FAssetIdentifier PropertyId = FAssetIdentifier(FLiveConfigProperty::StaticStruct(), RelatedName);
					TArray<FAssetIdentifier> Referencers;
					AssetRegistry.GetReferencers(PropertyId, Referencers, UE::AssetRegistry::EDependencyCategory::SearchableName);

					if (Referencers.Num() > 0)
					{
						bOldNameUsed = true;
						break;
					}

					// Config check
					FString PropertyString = RelatedName.ToString();
					for (const FString& ConfigValue : AllConfigValues)
					{
						if (ConfigValue.Contains(PropertyString))
						{
							bOldNameUsed = true;
							break;
						}
					}
					if (bOldNameUsed) break;
				}
			}

			if (!bOldNameUsed)
			{
				OutUnusedRedirects.Add(OldName);
			}
#else
			OutUnusedRedirects.Add(OldName);
#endif
		}
	}
}

bool ULiveConfigSystem::IsPropertyReferencedInAssets(FName PropertyName, TArray<FString>* OutReferencingAssets) const
{
#if WITH_EDITOR
	if (OutReferencingAssets)
	{
		OutReferencingAssets->Empty();
	}

	// Get the Asset Registry module
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FName> RelatedNames;
	GetRelatedPropertyNames(PropertyName, RelatedNames);

	bool bIsReferenced = false;
	for (const FName& NameToCheck : RelatedNames)
	{
		// Use SearchableNames in the Asset Registry to find references efficiently
		// This matches what was done in FLiveConfigProperty::PostSerialize
		FAssetIdentifier PropertyId = FAssetIdentifier(FLiveConfigProperty::StaticStruct(), NameToCheck);
		TArray<FAssetIdentifier> Referencers;
		AssetRegistry.GetReferencers(PropertyId, Referencers, UE::AssetRegistry::EDependencyCategory::SearchableName);

		if (Referencers.Num() > 0)
		{
			bIsReferenced = true;
			if (OutReferencingAssets)
			{
				for (const FAssetIdentifier& Referencer : Referencers)
				{
					OutReferencingAssets->AddUnique(Referencer.PackageName.ToString());
				}
			}
			else
			{
				// If we don't need the list, we can stop early
				return true;
			}
		}
	}

	return bIsReferenced;
#else
	return false;
#endif
}

void ULiveConfigSystem::RemoveRedirect(FName OldPropertyName)
{
	if (PropertyRedirects.Remove(OldPropertyName) > 0)
	{
		TryUpdateDefaultConfigFile();
		UE_LOG(LogLiveConfig, Log, TEXT("Removed redirect: %s"), *OldPropertyName.ToString());
	}
}

int32 ULiveConfigSystem::CleanupUnusedRedirects()
{
	TArray<FName> UnusedRedirects;
	GetUnusedRedirects(UnusedRedirects);

	for (const FName& RedirectKey : UnusedRedirects)
	{
		PropertyRedirects.Remove(RedirectKey);
	}

	if (UnusedRedirects.Num() > 0)
	{
		TryUpdateDefaultConfigFile();
		UE_LOG(LogLiveConfig, Log, TEXT("Cleaned up %d unused redirects"), UnusedRedirects.Num());
	}

	return UnusedRedirects.Num();
}

void ULiveConfigSystem::GetRelatedPropertyNames(FName PropertyName, TArray<FName>& OutRelatedNames) const
{
	OutRelatedNames.AddUnique(PropertyName);

	// Find the ultimate target if PropertyName is an old name
	FName CurrentTarget = PropertyName;
	while (PropertyRedirects.Contains(CurrentTarget))
	{
		CurrentTarget = PropertyRedirects[CurrentTarget];
		OutRelatedNames.AddUnique(CurrentTarget);
	}

	// Now we have the final target, let's find all names that eventually redirect to it
	TArray<FName> ToProcess;
	ToProcess.Add(CurrentTarget);

	int32 Index = 0;
	while (Index < ToProcess.Num())
	{
		FName Target = ToProcess[Index++];
		for (const auto& Pair : PropertyRedirects)
		{
			if (Pair.Value == Target)
			{
				if (!OutRelatedNames.Contains(Pair.Key))
				{
					OutRelatedNames.Add(Pair.Key);
					ToProcess.Add(Pair.Key);
				}
			}
		}
	}
}

void ULiveConfigSystem::HandleTagsChanged()
{
	TryUpdateDefaultConfigFile();
	OnTagsChanged.Broadcast();
}

void ULiveConfigSystem::DownloadConfig()
{
    if (IsRunningCookCommandlet())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Ignoring live config during cooking"));
        return;
    }
 
    if (CurrentRequest && !CurrentRequest->GetResponse())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Live config request already in progress, not downloading"));
        return;
    }

    double TimeSinceLastDownload = FPlatformTime::Seconds() - TimeLoadStarted;
    if (TimeSinceLastDownload < RateLimitSeconds)
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Preventing downloading config due to rate limit, time since last download %f (limit %f)"), TimeSinceLastDownload, RateLimitSeconds);
        return;
    }
    
    // need to sanitize sheet url so that it definitely has /export?format=csv
    // we may or may not want to specify the GID, depending on if the tab we're using is the first or not
    // could have GID as an optional arg, but this also gets confusing? Just going to assume it's the first tab for now
    if (SheetUrl.IsEmpty())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: SheetUrl is empty. No remote overrides will be downloaded"));
        return;
    }

    FHttpModule& HttpModule = FHttpModule::Get();
    CurrentRequest = HttpModule.CreateRequest();

    CurrentRequest->OnProcessRequestComplete().BindUObject(this, &ThisClass::OnSheetDownloadComplete);
    CurrentRequest->SetURL(SheetUrl);
    CurrentRequest->SetVerb(TEXT("GET"));
    CurrentRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
    CurrentRequest->SetHeader(TEXT("Content-Type"), TEXT("text/csv"));
    
    UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: Starting download from %s"), *SheetUrl);
    CurrentRequest->ProcessRequest();
    TimeLoadStarted = FPlatformTime::Seconds();
}

void ULiveConfigSystem::OnSheetDownloadComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogLiveConfig, Error, TEXT("LiveConfigSystem: Download failed."));
        return;
    }

    ULiveConfigSettings* GameSettings = GetMutableDefault<ULiveConfigSettings>();
    if (!GameSettings)
    {
        return;
    }

    const FString CsvContent = Response->GetContentAsString();
    
    FCsvParser Parser(CsvContent);
    const FCsvParser::FRows& Rows = Parser.GetRows();

    FLiveConfigProfile NewEnvProfile;
    // Start from index 1 to skip the header row
    for (int32 i = 1; i < Rows.Num(); ++i)
    {
        const TArray<const TCHAR*>& Columns = Rows[i];

        // We expect four columns: key, value, tags, description, but only actually need key/value
        if (Columns.Num() >= 1)
        {
            const FName Key(Columns[0]);
        	
            FLiveConfigProperty Property(Key, true);
            if (!PropertyDefinitions.Contains(Property))
            {
                UE_LOG(LogLiveConfig, Warning, TEXT("Skipping remote property with invalid key: %s"), *Property.ToString());
                continue;
            }
            
            FLiveConfigPropertyDefinition Def = PropertyDefinitions[Property];
            Def.Value = Columns[1];
            
            if (Property.IsValid() && !Def.Value.IsEmpty())
            {
                UE_LOG(LogLiveConfig, Verbose, TEXT("Downloaded config value: %s: %s (%s)"), *Property.ToString(), *Def.Value, *Def.Description);
                NewEnvProfile.Overrides.Add(Property, Def.Value);
            }
        }
    }
    
    bIsDataReady = true;
    UE_LOG(LogLiveConfig, Log, TEXT("Successfully loaded %d key-value pairs"), PropertyDefinitions.Num());
    
   	if (NewEnvProfile != EnvironmentOverrides)
	{   
		UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem:Environment profile changed, rebuilding cache"));
		EnvironmentOverrides = MoveTemp(NewEnvProfile);
		RebuildConfigCache();
	}
}

const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& ULiveConfigSystem::GetAllProperties() const
{
    return PropertyDefinitions;
}

void ULiveConfigSystem::OnTravel(UWorld* World, FWorldInitializationValues WorldInitializationValues)
{
    DownloadConfig();
}

void ULiveConfigSystem::OnStartGameInstance(UGameInstance* GameInstance)
{
    float PollingRate = ULiveConfigSettings::StaticClass()->GetDefaultObject<ULiveConfigSettings>()->PollingRate;
    
#if WITH_EDITOR
    PollingRate = ULiveConfigSettings::StaticClass()->GetDefaultObject<ULiveConfigSettings>()->EditorPollRateMinutes * 60;
#endif
    
    GameInstance->GetTimerManager().SetTimer(PollingTimer, FTimerDelegate::CreateWeakLambda(this, [&]
    {
        DownloadConfig();
    }), PollingRate, true);
}

void ULiveConfigSystem::BuildCache()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_LiveConfig_BuildCache);
	
	if (ULiveConfigProfileSystem* ProfileSystem = ULiveConfigProfileSystem::Get())
    {
        FLiveConfigCache::BuildConfig(PropertyDefinitions, EnvironmentOverrides, ProfileSystem->GetActiveProfile(), Cache);
    }
    else
    {
        FLiveConfigCache::BuildConfig(PropertyDefinitions, EnvironmentOverrides, {}, Cache);
    }
}

FString ULiveConfigSystem::GetStringValue(FLiveConfigProperty Key) const
{
    return Cache.GetValue<FString>(Key);
}

float ULiveConfigSystem::GetFloatValue(FLiveConfigProperty Key) const
{
    return Cache.GetValue<float>(Key);
}

int32 ULiveConfigSystem::GetIntValue(FLiveConfigProperty Key) const
{
    return Cache.GetValue<int32>(Key);
}

bool ULiveConfigSystem::GetBoolValue(FLiveConfigProperty Key) const
{
    return Cache.GetValue<bool>(Key);
}

void ULiveConfigSystem::GetLiveConfigStruct_Internal(UScriptStruct* Struct, void* OutStructPtr, FLiveConfigProperty Prefix) const
{
	if (!Struct || !OutStructPtr)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Prop = *It;
		
		// we save properties using GetAuthoredName, so we also look them up that way
		FString PropName = Prop->GetAuthoredName();
		FString FullPropName = Prefix.ToString() + TEXT(".") + PropName;
		FLiveConfigProperty ConfigProp(FullPropName, true);

		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
		{
 		double Value = Cache.GetValue<float>(ConfigProp);
			DoubleProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
		{
			float Value = Cache.GetValue<float>(ConfigProp);
			FloatProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			int32 Value = Cache.GetValue<int32>(ConfigProp);
			IntProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			bool Value = Cache.GetValue<bool>(ConfigProp);
			BoolProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			FString Value = Cache.GetValue<FString>(ConfigProp);
			StrProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			FName Value = FName(*Cache.GetValue<FString>(ConfigProp));
			NameProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			FText Value = FText::FromString(Cache.GetValue<FString>(ConfigProp));
			TextProp->SetPropertyValue_InContainer(OutStructPtr, Value);
		}
		else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		{
			FString Value = Cache.GetValue<FString>(ConfigProp);
			if (UEnum* Enum = EnumProp->GetEnum())
			{
				int64 EnumValue = Enum->GetValueByNameString(Value);
				if (EnumValue != INDEX_NONE)
				{
					EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(EnumProp->ContainerPtrToValuePtr<void>(OutStructPtr), EnumValue);
				}
			}
		}
	}
}

void ULiveConfigSystem::PopulateAutoCompleteEntries(TArray<FAutoCompleteCommand>& Entries)
{
    // Use iterator to avoid copying the full property list 
	for (auto Iter = PropertyDefinitions.CreateConstIterator(); Iter; ++Iter)
	{
		FLiveConfigProperty Prop = Iter->Key;
		auto Def = ULiveConfigLib::GetLiveConfigPropertyDefinition(Prop);
    	
    	// skip structs since we can set their subproperties individually
    	if (!Def.IsValid() || Def.PropertyType == ELiveConfigPropertyType::Struct)
    	{
    		continue;
    	}
    	
        FAutoCompleteCommand Command;
        Command.Command = FString::Printf(TEXT("LiveConfig.SetOverride %s"), *Prop.ToString());
        Command.Desc = FString::Printf(TEXT("Set live config override for %s (current %s)"), *Prop.ToString(), *Def.Value);
        Entries.Add(Command);
    }
}

