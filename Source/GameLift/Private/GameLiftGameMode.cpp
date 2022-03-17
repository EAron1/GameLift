// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameLiftGameMode.h"
#include "GameLiftCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGameLiftGameMode::AGameLiftGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AGameLiftGameMode::BeginPlay()
{
	Super::BeginPlay();
	
#if WITH_GAMELIFT
	
	auto InitSDKOutcome = Aws::GameLift::Server::InitSDK();

	if (InitSDKOutcome.IsSuccess())
	{
		auto OnStartGameSession = [](Aws::GameLift::Server::Model::GameSession GameSessionObj, void* Params)
		{
			FStartGameSessionState* State = (FStartGameSessionState*)Params;

			State->bStatus = Aws::GameLift::Server::ActivateGameSession().IsSuccess();
		};

		auto OnUpdateGameSession = [](Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSessionObj, void* Params)
		{
			FUpdateGameSessionState* State = (FUpdateGameSessionState*)Params;
		};

		auto OnProcessTerminate = [](void* Params)
		{
			FProcessTerminationState* State = (FProcessTerminationState*)Params;
			auto GetTerminationTimeOutcome = Aws::GameLift::Server::GetTerminationTime();
			if (GetTerminationTimeOutcome.IsSuccess())
			{
				State->TerminationTime = GetTerminationTimeOutcome.GetResult();
			}

			auto ProcessEndingOutcome = Aws::GameLift::Server::ProcessEnding();
			if (ProcessEndingOutcome.IsSuccess())
			{
				State->bStatus = true;
				FGenericPlatformMisc::RequestExit(false);
			}
		};

		auto OnHealthCheck = [](void* Params)
		{
			FHealthCheckState* State = (FHealthCheckState*)Params;
			State->bStatus = true;

			return State->bStatus;
		};

		TArray<FString> CommandLineTokens;
		TArray<FString> CommandLineSwitches;
		int Port = FURL::UrlConfig.DefaultPort;

		// GameLiftServer.exe token -port=7777
		FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

		for (FString Str : CommandLineSwitches)
		{
			FString Key;
			FString Value;
			if (Str.Split("=", &Key, &Value))
			{
				if (Key.Equals("port"))
				{
					Port = FCString::Atoi(*Value);
				}
			}
		}

		const char* LogFile = "aLogFile.txt";
		const char** LogFiles = &LogFile;
		auto LogParams = new Aws::GameLift::Server::LogParameters(LogFiles, 1);

		auto Params = new Aws::GameLift::Server::ProcessParameters(
			OnStartGameSession,
			&StartGameSessionState,
			OnUpdateGameSession,
			&UpdateGameSessionState,
			OnProcessTerminate,
			&ProcessTerminationState,
			OnHealthCheck,
			&HealthCheckState,
			Port,
			*LogParams
		);

		auto ProcessReadyOutcome = Aws::GameLift::Server::ProcessReady(*Params);


	}

#endif

}
