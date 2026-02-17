// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveConfigProfile.h"
#include "LiveConfigPlayerControllerComponent.generated.h"

/**
 * Component that should be added to the PlayerController to handle Server RPCs for Live Config.
 * This is needed because the ALiveConfigProfileActor does not have a net owning connection for clients.
 */
UCLASS(ClassGroup=(LiveConfig), meta=(BlueprintSpawnableComponent))
class LIVECONFIG_API ULiveConfigPlayerControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULiveConfigPlayerControllerComponent();

	/**
	 * Server-side: Set the active profile with full data. 
	 * This RPC is routed through the PlayerController's component to ensure it has a net owning connection.
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetActiveProfileData(FLiveConfigProfile Profile);
};
