// Copyright (c) 2026 Nicholas Arthur
// Licensed under the MIT License

#include "Profiles/LiveConfigProfileActor.h"

#include "EngineUtils.h"
#include "LiveConfigSystem.h"
#include "Net/UnrealNetwork.h"
#include "Profiles/LiveConfigProfileSystem.h"

ALiveConfigProfileActor::ALiveConfigProfileActor()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    
    bNetLoadOnClient = false;
    
    // Very slow update frequency 
    SetNetUpdateFrequency(1.f);
    
    AActor::SetReplicateMovement(false);
}

void ALiveConfigProfileActor::BeginPlay()
{
    Super::BeginPlay();
    
    if (GetWorld()->GetNetMode() == NM_Client)
    {
        UE_LOG(LogLiveConfig, Log, TEXT("Live Config Profile Actor spawned on client"));
    }
}

ALiveConfigProfileActor* ALiveConfigProfileActor::Get(const UObject* WorldContext)
{
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
    
    if (!World)
    {
        return nullptr;
    }
    
    // There should only be one 
    if (TActorIterator<ALiveConfigProfileActor> ActorIterator(World, StaticClass()); ActorIterator)
    {
        return *ActorIterator;
    }
    
    return nullptr;
}

void ALiveConfigProfileActor::ServerSetActiveProfile(FName ProfileName)
{
    if (!HasAuthority())
    {
        return;
    }

    if (ProfileName == NAME_None)
    {
        ReplicatedActiveProfile = FLiveConfigProfile();
    }
    else
    {
        if (ULiveConfigProfileSystem* System = ULiveConfigProfileSystem::Get())
        {
            System->LoadProfile(ProfileName, ReplicatedActiveProfile);
        }
    }
    
    ReplicatedActiveProfile.PrepareForReplication();
    ApplyProfileToSystem();
}

void ALiveConfigProfileActor::ServerSetActiveProfileData_Implementation(const FLiveConfigProfile& Profile)
{
    ReplicatedActiveProfile = Profile;
    
    // If we're coming from the PlayerControllerComponent, the data is already unpacked into Overrides (via OnAfterReplication)
    // but ReplicatedActiveProfile.ReplicatedOverrides is what actually replicates to OTHER clients.
    // So we need to ensure ReplicatedOverrides is populated.
    ReplicatedActiveProfile.PrepareForReplication();
    
    ApplyProfileToSystem();
}

bool ALiveConfigProfileActor::ServerSetActiveProfileData_Validate(const FLiveConfigProfile& Profile)
{
    return true;
}

void ALiveConfigProfileActor::OnRep_ActiveProfile()
{
    ReplicatedActiveProfile.OnAfterReplication();
    ApplyProfileToSystem();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void ALiveConfigProfileActor::ApplyProfileToSystem()
{
    if (ULiveConfigProfileSystem* System = ULiveConfigProfileSystem::Get())
    {
        System->SetActiveProfileData_NoReplication(ReplicatedActiveProfile);
    }
}

void ALiveConfigProfileActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALiveConfigProfileActor, ReplicatedActiveProfile);
}
