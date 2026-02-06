#include "Profiles/LiveConfigProfileReplicationSystem.h"
#include "Profiles/LiveConfigProfileActor.h"
#include "Profiles/LiveConfigPlayerControllerComponent.h"
#include "LiveConfigSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Profiles/LiveConfigProfileSystem.h"

void ULiveConfigProfileReplicationSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (GetWorld())
    {
        ActorSpawnedDelegateHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ULiveConfigProfileReplicationSystem::OnActorSpawned));
    }
}

void ULiveConfigProfileReplicationSystem::Deinitialize()
{
    if (GetWorld() && ActorSpawnedDelegateHandle.IsValid())
    {
        GetWorld()->RemoveOnActorSpawnedHandler(ActorSpawnedDelegateHandle);
    }

    Super::Deinitialize();
}

void ULiveConfigProfileReplicationSystem::OnActorSpawned(AActor* InActor)
{
    if (APlayerController* PC = Cast<APlayerController>(InActor))
    {
        // Add the component if it doesn't exist
        if (!PC->FindComponentByClass<ULiveConfigPlayerControllerComponent>())
        {
            ULiveConfigPlayerControllerComponent* NewComp = NewObject<ULiveConfigPlayerControllerComponent>(PC, ULiveConfigPlayerControllerComponent::StaticClass(), TEXT("LiveConfigPlayerControllerComponent"));
            if (NewComp)
            {
                NewComp->CreationMethod = EComponentCreationMethod::Instance;
                NewComp->RegisterComponent();
            }
        }
    }
}

void ULiveConfigProfileReplicationSystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client)
    {
        ULiveConfigProfileSystem::Get()->SetActiveProfileData_NoReplication({});
        return;
    }
    
    if (InWorld.GetNetMode() == NM_Standalone)
    {
        return;
    }

    const ULiveConfigSettings* Settings = GetDefault<ULiveConfigSettings>();
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
    if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (ULiveConfigPlayerControllerComponent* Comp = PC->FindComponentByClass<ULiveConfigPlayerControllerComponent>())
            {
                FLiveConfigProfile ProfileCopy = Profile;
                ProfileCopy.PrepareForReplication();
                Comp->ServerSetActiveProfileData(ProfileCopy);
                return true;
            }

            // Fallback if component is missing but we have authority
            if (PC->HasAuthority())
            {
                if (ALiveConfigProfileActor* Actor = ALiveConfigProfileActor::Get(WorldContext))
                {
                    Actor->ServerSetActiveProfileData(Profile);
                    return true;
                }
            }
        }
    }

    ALiveConfigProfileActor* Actor = ALiveConfigProfileActor::Get(WorldContext);
    if (!Actor)
    {
        return false;
    }
    
    Actor->ServerSetActiveProfileData(Profile);
    return true;
}

