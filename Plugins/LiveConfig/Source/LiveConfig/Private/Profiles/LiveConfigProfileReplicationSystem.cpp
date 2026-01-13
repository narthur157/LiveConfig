#include "Profiles/LiveConfigProfileReplicationSystem.h"
#include "Profiles/LiveConfigProfileActor.h"
#include "LiveConfigGameSettings.h"
#include "Engine/World.h"

void ULiveConfigProfileReplicationSystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client || InWorld.GetNetMode() == NM_Standalone)
    {
        return;
    }

    const ULiveConfigGameSettings* Settings = GetDefault<ULiveConfigGameSettings>();
    if (Settings && Settings->bEnableProfileReplication)
    {
        TSubclassOf<ALiveConfigProfileActor> ActorClass = ALiveConfigProfileActor::StaticClass();
        if (Settings->ProfileActorClass.IsValid())
        {
            if (UClass* LoadedClass = Settings->ProfileActorClass.TryLoadClass<ALiveConfigProfileActor>())
            {
                ActorClass = LoadedClass;
            }
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = TEXT("LiveConfigProfileActor");
        SpawnParams.ObjectFlags |= RF_Transient;
        
        InWorld.SpawnActor<ALiveConfigProfileActor>(ActorClass, SpawnParams);
    }
}

bool ULiveConfigProfileReplicationSystem::ReplicateProfileData(const FLiveConfigProfile& Profile, const UObject* WorldContext)
{
    ALiveConfigProfileActor* Actor = ALiveConfigProfileActor::Get(WorldContext);
    if (!Actor)
    {
        return false;
    }
    
    Actor->ServerSetActiveProfileData(Profile);
    return true;
}
