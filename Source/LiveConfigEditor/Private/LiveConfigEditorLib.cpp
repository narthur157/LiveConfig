// Fill out your copyright notice in the Description page of Project Settings.


#include "LiveConfigEditorLib.h"

#include "LiveConfigSystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/AssetRegistryInterface.h"

void ULiveConfigEditorLib::GetUnusedProperties(TArray<FName>& OutUnusedProperties)
{
	OutUnusedProperties.Empty();

	for (const auto& Pair : ULiveConfigSystem::Get().PropertyDefinitions)
	{
		const FName PropertyName = Pair.Key.GetName();
		if (!IsPropertyNameUsed(PropertyName))
		{
			OutUnusedProperties.Add(PropertyName);
		}
	}
}

void ULiveConfigEditorLib::GetUnusedRedirects(TArray<FName>& OutUnusedRedirects)
{
	OutUnusedRedirects.Empty();

	const TMap<FName, FName>& PropertyRedirects = ULiveConfigSystem::Get().PropertyRedirects;
	const TMap<FLiveConfigProperty, FLiveConfigPropertyDefinition>& PropertyDefinitions = ULiveConfigSystem::Get().
	PropertyDefinitions;
	
	for (const auto& RedirectPair : PropertyRedirects)
	{
		const FName& OldName = RedirectPair.Key;
		const FName& NewName = RedirectPair.Value;

		// Check if the target property exists
		FLiveConfigProperty TargetProperty(NewName);
		if (!PropertyDefinitions.Contains(TargetProperty))
		{
			// Even if target is missing, if the OLD name is still used, we might want to keep the redirect?
			if (!IsPropertyNameUsed(OldName))
			{
				OutUnusedRedirects.Add(OldName);
			}
		}
	}
}

bool ULiveConfigEditorLib::IsPropertyNameUsed(FName PropertyName)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FName> NamesToCheck;
	
	// check if redirectors are in use as well 
	GetRelatedPropertyNames(PropertyName, NamesToCheck);

	for (const FName& NameToCheck : NamesToCheck)
	{
		// Check for references to this property in assets
		FAssetIdentifier PropertyId = FAssetIdentifier(FLiveConfigProperty::StaticStruct(), NameToCheck);
		TArray<FAssetIdentifier> ReferencedBy;
		AssetRegistry.GetReferencers(PropertyId, ReferencedBy, UE::AssetRegistry::EDependencyCategory::SearchableName);

		if (ReferencedBy.Num() > 0)
		{
			return true;
		}
	}
	return false;
}

int32 ULiveConfigEditorLib::CleanupUnusedRedirects()
{
	TArray<FName> UnusedRedirects;
	GetUnusedRedirects(UnusedRedirects);

	TMap<FName, FName>& PropertyRedirects = ULiveConfigSystem::Get().PropertyRedirects;
	for (const FName& RedirectKey : UnusedRedirects)
	{
		PropertyRedirects.Remove(RedirectKey);
	}

	if (UnusedRedirects.Num() > 0)
	{
		ULiveConfigSystem::Get().TryUpdateDefaultConfigFile();
		UE_LOG(LogLiveConfig, Log, TEXT("Cleaned up %d unused redirects"), UnusedRedirects.Num());
	}

	return UnusedRedirects.Num();
}

void ULiveConfigEditorLib::GetRelatedPropertyNames(FName PropertyName, TArray<FName>& OutRelatedNames)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_LiveConfig_GetRelatedPropertyNames);
	
	OutRelatedNames.AddUnique(PropertyName);

	// Find the ultimate target if PropertyName is an old name
	FName CurrentTarget = PropertyName;
	
	const TMap<FName, FName>& PropertyRedirects = ULiveConfigSystem::Get().PropertyRedirects;
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
