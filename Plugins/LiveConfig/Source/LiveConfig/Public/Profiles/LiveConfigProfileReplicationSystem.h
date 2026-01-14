#pragma once

#include "CoreMinimal.h"
#include "LiveConfigProfile.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "LiveConfigProfileReplicationSystem.generated.h"

class ALiveConfigProfileActor;

/**
 * World subsystem to manage the Live Config profile replication actor.
 */
UCLASS()
class LIVECONFIG_API ULiveConfigProfileReplicationSystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Gets the profile actor and calls set profile data on that
     */
    static bool ReplicateProfileData(const FLiveConfigProfile& Profile, const UObject* WorldContext);

private:
    void OnActorSpawned(AActor* InActor);

    FDelegateHandle ActorSpawnedDelegateHandle;
};
