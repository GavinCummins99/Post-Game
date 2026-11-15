// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ParcelGameGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */
UCLASS(abstract)
class AParcelGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AParcelGameGameMode();
};



