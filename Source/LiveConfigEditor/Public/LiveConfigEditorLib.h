// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LiveConfigEditorLib.generated.h"

/**
 * 
 */
UCLASS()
class LIVECONFIGEDITOR_API ULiveConfigEditorLib : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Gets all properties that are not referenced in any assets or config files
	 * @param OutUnusedProperties Array to fill with unused property names
	 */

	static void GetUnusedProperties(TArray<FName>& OutUnusedProperties);
	/**
	 * Gets all redirects that point to properties that no longer exist
	 * @param OutUnusedRedirects Array to fill with redirect keys (old names) that point to non-existent properties and are not referenced
	 */
	static void GetUnusedRedirects(TArray<FName>& OutUnusedRedirects);

	/**
	 * Removes all redirects that point to properties that no longer exist
	 * @return Number of redirects removed
	 */
	static int32 CleanupUnusedRedirects();

	/**
	 * Gets all names related to a property, including redirects (old names)
	 * @param PropertyName The name to find related names for
	 * @param OutRelatedNames Array to fill with related names
	 */
	static void GetRelatedPropertyNames(FName PropertyName, TArray<FName>& OutRelatedNames);

	/** Internal helper to check if a property name or its related names are used in assets or configs */
	static bool IsPropertyNameUsed(FName PropertyName);
	
};
