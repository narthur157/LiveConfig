// Copyright 2026 Nicholas Arthur - All Rights Reserved

#include "LiveConfigTestActor.h"
#include "LiveConfigLib.h"


ALiveConfigTestActor::ALiveConfigTestActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALiveConfigTestActor::BeginPlay()
{
	Super::BeginPlay();
	
	bool bValue = ULiveConfigLib::GetLiveConfigValue<bool>("LiveConfigTest");
	UE_LOG(LogLiveConfig, Log, TEXT("Live config test actor got value %s for LiveConfigTest"), bValue ? TEXT("true") : TEXT("false"));	
}