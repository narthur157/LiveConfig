// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "Profiles/LiveConfigProfile.h"
#include "LiveConfigSystem.h"


void FLiveConfigProfile::Redirect()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_LiveConfigProfiles_Redirect);
	
	TMap<FLiveConfigProperty, FString> NewOverrides;
	
	for (auto& Pair : Overrides)
	{
		FLiveConfigProperty Key = Pair.Key;
		ULiveConfigSystem::Get().RedirectPropertyName(Key);
		NewOverrides.Add(Key, Pair.Value);
	}
	
	Overrides = MoveTemp(NewOverrides);
}
