// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameLiftServerSDK.h"
#include "GameFramework/GameModeBase.h"
#include "GameLiftGameMode.generated.h"

USTRUCT()
struct FStartGameSessionState
{
	GENERATED_BODY();

	FStartGameSessionState()
	{
		bStatus = false;
	}

	UPROPERTY()
	bool bStatus;

};

USTRUCT()
struct FUpdateGameSessionState
{
	GENERATED_BODY();

	FUpdateGameSessionState()
	{
	}

};

USTRUCT()
struct FProcessTerminationState
{
	GENERATED_BODY();

	FProcessTerminationState()
	{
		bStatus = false;
	}

	UPROPERTY()
	bool bStatus;

	long TerminationTime;

};

USTRUCT()
struct FHealthCheckState
{
	GENERATED_BODY();

	FHealthCheckState()
	{
		bStatus = false;
	}

	UPROPERTY()
	bool bStatus;

};

UCLASS(minimalapi)
class AGameLiftGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AGameLiftGameMode();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	FStartGameSessionState StartGameSessionState;
	
	UPROPERTY()
	FUpdateGameSessionState UpdateGameSessionState;

	UPROPERTY()
	FProcessTerminationState ProcessTerminationState;

	UPROPERTY()
	FHealthCheckState HealthCheckState;

};



