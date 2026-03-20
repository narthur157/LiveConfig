// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "LiveConfigSystem.h"

#include "Profiles/LiveConfigProfileSystem.h"
#include "LiveConfigSettings.h"
#include "LiveConfigUserSettings.h"
#include "LiveConfigTypes.h"

#include "LiveConfigJson.h"
#include "ConsoleSettings.h"
#include "LiveConfigCurveTableUpdater.h"
#include "LiveConfigLib.h"
#include "Engine/Console.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"

namespace LiveConfigTags
{
	const FName FromCurveTable = TEXT("FromCurveTable");
}

static TArray<FFieldClass*> SupportedStructPropertyTypes = {
	FDoubleProperty::StaticClass(), FFloatProperty::StaticClass(), FIntProperty::StaticClass(),
	FBoolProperty::StaticClass(), FStrProperty::StaticClass(), FNameProperty::StaticClass(), 
	FTextProperty::StaticClass(), FEnumProperty::StaticClass()
};

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

    UConsole::RegisterConsoleAutoCompleteEntries.AddUObject(this, &ThisClass::PopulateAutoCompleteEntries);

   	if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
	{
		JsonSystem->LoadJsonFromFiles();
	}

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("LiveConfig.SyncRemote"),
		TEXT("Fetches the latest remote config and updates local JSON files if changes are detected."),
		FConsoleCommandDelegate::CreateUObject(this, &ULiveConfigSystem::SyncRemoteToLocal),
		ECVF_Default
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("LiveConfig.FetchProfile"),
		TEXT("Fetches a profile from URL or file path. Auto-detects: http/https = remote, otherwise = file path"),
		FConsoleCommandWithArgsDelegate::CreateUObject(this, &ULiveConfigSystem::FetchProfileCommand),
		ECVF_Default
	);

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
	
	IConsoleManager::Get().UnregisterConsoleObject(TEXT("LiveConfig.FetchProfile"));
	IConsoleManager::Get().UnregisterConsoleObject(TEXT("LiveConfig.SyncRemote"));
}

bool ULiveConfigSystem::ShouldCreateSubsystem(UObject* Outer) const
{
    return true;
}

void ULiveConfigSystem::SaveProperty(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
	ULiveConfigJsonSystem::Get()->SavePropertyToFile(PropertyDefinition);
	
	if (PropertyDefinition.PropertyType == ELiveConfigPropertyType::Struct)
	{
		UpdateStructProperties(PropertyDefinition);
	}
	
	GEngine->GetEngineSubsystem<ULiveConfigCurveTableUpdater>()->UpdateCurveTables();
	RebuildConfigCache();
}

bool ULiveConfigSystem::UpdateStructProperties(const FLiveConfigPropertyDefinition& PropertyDefinition)
{
	UScriptStruct* Struct = FindFirstObject<UScriptStruct>(*PropertyDefinition.Value);
	if (!Struct)
	{
		return false;
	}

	FString Prefix = PropertyDefinition.PropertyName.ToString();
	
	if (Prefix.IsEmpty() || Prefix.EndsWith(TEXT(".")))
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("UpdateStructProperties: Prefix is empty or ends with dot, skipping generation"));
		return false;
	}

	bool bChanged = false;
	
	TArray<FLiveConfigProperty> OldSubProperties;
	GetSubProperties(PropertyDefinition.PropertyName.GetName(), OldSubProperties);

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Prop = *It;
		FString FullPropName = Prefix + TEXT(".") + Prop->GetAuthoredName();
		FLiveConfigProperty ConfigProp(FullPropName);

		ELiveConfigPropertyType PropType = ELiveConfigPropertyType::String;
		FString DefaultValue = "";

		if (CastField<FDoubleProperty>(Prop) || CastField<FFloatProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Float;
			DefaultValue = "0";
		}
		else if (CastField<FIntProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Int;
			DefaultValue = "0";
		}
		else if (CastField<FBoolProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::Bool;
			DefaultValue = "false";
		}
		else if (CastField<FStrProperty>(Prop) || CastField<FNameProperty>(Prop) || CastField<FTextProperty>(Prop) || CastField<FEnumProperty>(Prop))
		{
			PropType = ELiveConfigPropertyType::String;
			DefaultValue = "";
		}
		else
		{
			UE_LOG(LogLiveConfig, Log, TEXT("UpdateStructProperties: Skipping unsupported property type for %s"), *Prop->GetName());
			continue;
		}
		
		if (PropertyDefinitions.Contains(ConfigProp) && PropertyDefinitions[ConfigProp].PropertyType == PropType)
		{
			// if we don't need to remove a property, don't
			OldSubProperties.Remove(ConfigProp);
			
			UE_LOG(LogLiveConfig, Log, TEXT("UpdateStructProperties: Property %s already exists, skipping"), *FullPropName);
			continue;
		}

		FLiveConfigPropertyDefinition NewDef;
		NewDef.PropertyName = ConfigProp;
		NewDef.PropertyType = PropType;
		NewDef.Value = DefaultValue;
		NewDef.Tags = PropertyDefinition.Tags; // Inherit tags from parent struct property

		UE_LOG(LogLiveConfig, Log, TEXT("UpdateStructProperties: Adding property %s"), *FullPropName);
		SavePropertyDeferred(NewDef);
		
		bChanged = true;
	}
	
	// delete old sub properties now that we've had a chance to skip deleting any that are the same
	for (FLiveConfigProperty OldProp : OldSubProperties)
	{
		// DeleteProperty, except defer rebuilding the config cache
		PropertyDefinitions.Remove(OldProp);
	
		if (ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get())
		{
			JsonSystem->DeletePropertyFile(OldProp.PropertyName);
		}
	}
		
	PropertyDefinitions.Add(PropertyDefinition.PropertyName, PropertyDefinition);
	ULiveConfigJsonSystem::Get()->SavePropertyToFile(PropertyDefinition);
	
	if (bChanged)
	{
		RebuildConfigCache();
	}
	
	return bChanged;
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
		GetSubProperties(OldName.GetName(), MembersToRename);

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

void ULiveConfigSystem::DeleteProperty(FLiveConfigProperty Property)
{
	ULiveConfigJsonSystem* JsonSystem = ULiveConfigJsonSystem::Get();
	
	auto DeletePropertyInner = [&](FLiveConfigProperty InnerProp)
	{
		PropertyDefinitions.Remove(InnerProp);
	
		if (JsonSystem)
		{
			JsonSystem->DeletePropertyFile(InnerProp.PropertyName);
		}
	};
	
	if (FLiveConfigPropertyDefinition* Def = PropertyDefinitions.Find(Property))
	{
		if (Def->PropertyType == ELiveConfigPropertyType::Struct)
		{
			TArray<FLiveConfigProperty> SubProperties;			
			
			GetStructPropertyMembers(Property, SubProperties);
			for (FLiveConfigProperty StructProp : SubProperties)
			{
				DeletePropertyInner(StructProp);	
			}
		}
	}
	
	DeletePropertyInner(Property);
	
	RebuildConfigCache();
}


void ULiveConfigSystem::RemoveRedirect(FName OldPropertyName)
{
	if (PropertyRedirects.Remove(OldPropertyName) > 0)
	{
		TryUpdateDefaultConfigFile();
		UE_LOG(LogLiveConfig, Log, TEXT("Removed redirect: %s"), *OldPropertyName.ToString());
	}
}

void ULiveConfigSystem::HandleTagsChanged()
{
	TryUpdateDefaultConfigFile();
	OnTagsChanged.Broadcast();
}

void ULiveConfigSystem::PatchEnvironmentOverrides(const FLiveConfigProfile& InProfile)
{
	if (EnvironmentOverrides != InProfile)
	{
		for (TTuple<FLiveConfigProperty, FString> Pair : InProfile.Overrides)
		{
			if (!PropertyDefinitions.Contains(Pair.Key))
			{
				UE_LOG(LogLiveConfig, Warning, TEXT("Environment profile contains override %s not found in properties"), *Pair.Key.ToString());
			}
		}

		// Check for changed properties and notify editor subsystem
		TArray<FLiveConfigPropertyDefinition> ChangedProperties = GetProfileDiff(InProfile);
		if (ChangedProperties.Num() > 0)
		{
			// Broadcast to editor subsystem for notification handling
			OnDataProviderSync.Broadcast(ChangedProperties);
		}

		UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: Environment profile patched, rebuilding cache"));
		EnvironmentOverrides = InProfile;
		bIsDataReady = true;
		RebuildConfigCache();
		OnRemoteOverridesLoaded.Broadcast();
	}
	
	// If we loaded our overrides but there wasn't a change, we still finished loading
	if (!bIsDataReady)
	{
		bIsDataReady = true;
		OnRemoteOverridesLoaded.Broadcast();
	}
}

void ULiveConfigSystem::SyncRemoteToLocal()
{
	UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: Manually syncing remote to local..."));

	ELiveConfigSourceType SourceType;
	FString SourcePath;
	GetActiveSource(SourceType, SourcePath);
	
	FOnRemoteOverridesFetched OnComplete;
	OnComplete.BindWeakLambda(this, [this](const FLiveConfigProfile& InProfile)
	{
		TArray<FLiveConfigPropertyDefinition> Diff = GetProfileDiff(InProfile);
		PatchEnvironmentOverrides(InProfile);
		SaveProperties(Diff);
	});

	FetchOverrides(SourceType, SourcePath, OnComplete);
}

TArray<FLiveConfigPropertyDefinition> ULiveConfigSystem::GetProfileDiff(const FLiveConfigProfile& Profile)
{
	TArray<FLiveConfigPropertyDefinition> ChangedProperties;
	
	for (const TTuple<FLiveConfigProperty, FString>& Pair : Profile.Overrides)
	{
		if (FLiveConfigPropertyDefinition* LocalDef = PropertyDefinitions.Find(Pair.Key))
		{
			if (LocalDef->Value != Pair.Value)
			{
				FLiveConfigPropertyDefinition NewDef = *LocalDef;
				NewDef.Value = Pair.Value;
				ChangedProperties.Add(NewDef);
			}
		}
	}
	
	return ChangedProperties;
}

const TArray<FLiveConfigPropertyDefinition>& ULiveConfigSystem::SaveProperties(const TArray<FLiveConfigPropertyDefinition>& Properties)
{
#if WITH_EDITOR
	// In editor: Save changes to disk
	for (const auto& Def : Properties)
	{
		SavePropertyDeferred(Def);
	}
	
	UE_LOG(LogLiveConfig, Log, TEXT("Applied %d remote changes to local files"), Properties.Num());
#endif

	RebuildConfigCache();
	return Properties;
}

FString ULiveConfigSystem::GetDiffString(const TArray<FLiveConfigPropertyDefinition>& Diff)
{
	TArray<FString> PerProperty;
	
	for (const auto& Def : Diff)
	{
		FString DefString = FString::Format(TEXT("{0}: {1} -> {2}"), {Def.PropertyName.ToString(), PropertyDefinitions[Def.PropertyName].Value, Def.Value });
		PerProperty.Add(DefString);
	}
	
	return FString::Join(PerProperty, TEXT("\n"));
}

FText ULiveConfigSystem::GetDiffText(const TArray<FLiveConfigPropertyDefinition>& Diff)
{
	return FText::FromString(GetDiffString(Diff));
}

void ULiveConfigSystem::DownloadConfig()
{
    if (IsRunningCookCommandlet())
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Ignoring live config during cooking"));
        return;
    }
 
    double TimeSinceLastDownload = FPlatformTime::Seconds() - TimeLoadStarted;
    if (TimeSinceLastDownload < RateLimitSeconds)
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Preventing downloading config due to rate limit, time since last download %f (limit %f)"), TimeSinceLastDownload, RateLimitSeconds);
        return;
    }
    
    ELiveConfigSourceType SourceType;
    FString SourcePath;
    GetActiveSource(SourceType, SourcePath);
	
    if (SourceType != ELiveConfigSourceType::None)
    {
        UE_LOG(LogLiveConfig, Log, TEXT("LiveConfigSystem: Starting download from %s"), *SourcePath);

        FOnRemoteOverridesFetched OnComplete;
        OnComplete.BindUObject(this, &ULiveConfigSystem::PatchEnvironmentOverrides);
        FetchOverrides(SourceType, SourcePath, OnComplete);

        TimeLoadStarted = FPlatformTime::Seconds();
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
    float PollingRate = GetDefault<ULiveConfigSettings>()->PollingRate;

#if WITH_EDITOR
    // Use user-specific polling rate in editor
    const ULiveConfigUserSettings* UserSettings = GetDefault<ULiveConfigUserSettings>();
    if (UserSettings)
    {
        PollingRate = UserSettings->EditorPollRateMinutes * 60.0f;
    }
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

void ULiveConfigSystem::GetStructPropertyMembers(FLiveConfigProperty StructProperty,
	TArray<FLiveConfigProperty>& OutProperties)
{
	FLiveConfigPropertyDefinition* Definition = PropertyDefinitions.Find(StructProperty);
	if (!Definition || Definition->PropertyType != ELiveConfigPropertyType::Struct)
	{
		return;
	}
	
	// the value of a struct is its struct type
	UScriptStruct* Struct =  FindFirstObject<UScriptStruct>(*Definition->Value);;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		FProperty* Prop = *It;
		
		// we save properties using GetAuthoredName, so we also look them up that way
		FString PropName = Prop->GetAuthoredName();
		FString FullPropName = StructProperty.ToString() + TEXT(".") + PropName;
		FLiveConfigProperty ConfigProp(FullPropName, true);
		
		// TODO: We could check here if the prop is one of our supported struct types
		// though it's probably enough just to check that it exists in the first place so long as this isn't used
		// when determining which properties to create
		if (SupportedStructPropertyTypes.Contains(Prop->GetClass()))
		{
			OutProperties.Add(ConfigProp);
		}
	}
}

void ULiveConfigSystem::GetSubProperties(FName PropertyPath, TArray<FLiveConfigProperty>& OutProperties)
{
	FString PathStr = PropertyPath.ToString();
	if (PathStr.IsEmpty())
	{
		// refuse to fill the entire config
		return;
	}

	if (!PathStr.EndsWith(TEXT(".")))
	{
		PathStr += TEXT(".");
	}

	for (const TTuple<FLiveConfigProperty, FLiveConfigPropertyDefinition>& Pair : PropertyDefinitions)
	{
		FString PropName = Pair.Key.ToString();
		if (PropName.StartsWith(PathStr))
		{
			OutProperties.Add(Pair.Key);
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

void ULiveConfigSystem::FetchOverrides(ELiveConfigSourceType SourceType, const FString& SourcePath, const FOnRemoteOverridesFetched& OnComplete)
{
	if (SourceType == ELiveConfigSourceType::None || SourcePath.IsEmpty())
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("FetchOverrides: No source configured"));
		OnComplete.ExecuteIfBound(FLiveConfigProfile());
		return;
	}

	if (SourceType == ELiveConfigSourceType::HttpCsv)
	{
		TSharedRef<IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		HttpRequest->OnProcessRequestComplete().BindWeakLambda(this, [OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			FLiveConfigProfile Profile;
			if (bWasSuccessful && Response.IsValid())
			{
				const FString CsvContent = Response->GetContentAsString();
				Profile = ULiveConfigLib::ParseOverridesFromCsv(CsvContent);
				UE_LOG(LogLiveConfig, Log, TEXT("Successfully fetched HTTP CSV"));
			}
			else
			{
				UE_LOG(LogLiveConfig, Error, TEXT("HTTP CSV fetch failed"));
			}
			OnComplete.ExecuteIfBound(Profile);
		});

		HttpRequest->SetURL(SourcePath);
		HttpRequest->SetVerb(TEXT("GET"));
		HttpRequest->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("text/csv"));

		UE_LOG(LogLiveConfig, Log, TEXT("Fetching HTTP CSV from: %s"), *SourcePath);
		HttpRequest->ProcessRequest();
	}
	else if (SourceType == ELiveConfigSourceType::LocalCsv)
	{
		FLiveConfigProfile Profile;
		FString CsvContent;

		// FILEREAD_AllowWrite allows us to open the file even if LibreOffice has a write lock on it
		if (FFileHelper::LoadFileToString(CsvContent, *SourcePath, FFileHelper::EHashOptions::None, FILEREAD_AllowWrite))
		{
			Profile = ULiveConfigLib::ParseOverridesFromCsv(CsvContent);
			UE_LOG(LogLiveConfig, Log, TEXT("Successfully loaded local CSV from: %s"), *SourcePath);
		}
		else
		{
			FString AbsPath = FPaths::ConvertRelativePathToFull(SourcePath);
			UE_LOG(LogLiveConfig, Error, TEXT("Failed to load local CSV from: %s"), *AbsPath);
		}

		OnComplete.ExecuteIfBound(Profile);
	}
}

void ULiveConfigSystem::GetActiveSource(ELiveConfigSourceType& OutSourceType, FString& OutSourcePath)
{
	OutSourceType = ELiveConfigSourceType::None;
	
#if WITH_EDITOR
	const ULiveConfigUserSettings* UserSettings = GetDefault<ULiveConfigUserSettings>();
	if (UserSettings->ExternalSourceOverride != ELiveConfigSourceType::None)
	{
		OutSourceType = UserSettings->ExternalSourceOverride;
		OutSourcePath = ULiveConfigUserSettings::GetSourcePath();
		
		return;
	}
#endif

	const ULiveConfigSettings* ProjectSettings = GetDefault<ULiveConfigSettings>();
	if (ProjectSettings->DefaultExternalSource != ELiveConfigSourceType::None)
	{
		OutSourceType = ProjectSettings->DefaultExternalSource;
		OutSourcePath = ULiveConfigSettings::GetSourcePath();
	}
}

void ULiveConfigSystem::FetchProfileCommand(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogLiveConfig, Warning, TEXT("LiveConfig.FetchProfile requires a URL or file path argument"));
		return;
	}

	FString PathOrUrl = Args[0];

	// Auto-detect: http/https = HttpCsv, otherwise = LocalCsv
	ELiveConfigSourceType SourceType = ELiveConfigSourceType::LocalCsv;
	if (PathOrUrl.StartsWith(TEXT("http://")) || PathOrUrl.StartsWith(TEXT("https://")))
	{
		SourceType = ELiveConfigSourceType::HttpCsv;
		UE_LOG(LogLiveConfig, Log, TEXT("FetchProfile: Detected HTTP CSV source"));
	}
	else
	{
		UE_LOG(LogLiveConfig, Log, TEXT("FetchProfile: Detected local CSV source"));
	}

	FOnRemoteOverridesFetched OnComplete;
	OnComplete.BindUObject(this, &ULiveConfigSystem::PatchEnvironmentOverrides);
	FetchOverrides(SourceType, PathOrUrl, OnComplete);
}
