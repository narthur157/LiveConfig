#include "Profiles/LiveConfigPlayerControllerComponent.h"
#include "Profiles/LiveConfigProfileActor.h"

ULiveConfigPlayerControllerComponent::ULiveConfigPlayerControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULiveConfigPlayerControllerComponent::ServerSetActiveProfileData_Implementation(FLiveConfigProfile Profile)
{
	Profile.OnAfterReplication();
	if (ALiveConfigProfileActor* ProfileActor = ALiveConfigProfileActor::Get(this))
	{
		ProfileActor->ServerSetActiveProfileData(Profile);
	}
}

bool ULiveConfigPlayerControllerComponent::ServerSetActiveProfileData_Validate(FLiveConfigProfile Profile)
{
	return true;
}
