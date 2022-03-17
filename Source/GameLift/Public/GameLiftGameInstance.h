// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GameLiftGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class GAMELIFT_API UGameLiftGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:

	UGameLiftGameInstance();

	void Shutdown() override;

	void Init() override;

	UPROPERTY()
	FString AccessToken;

	UPROPERTY()
	FString IdToken;

	UPROPERTY()
	FString RefreshToken;

	UPROPERTY()
	FString MatchmakingTicketId;

	UPROPERTY()
	FTimerHandle RetrieveNewTokensHandle;
	
	UPROPERTY()
	FTimerHandle GetResponseTimeHandle;

	TDoubleLinkedList<float> PlayerLatencies;

	UFUNCTION()
	void SetCognitoTokens(FString inAccessToken, FString inIdToken, FString inRefreshToken);

private:

	FHttpModule* HttpModule;

	UPROPERTY()
	FString ApiUrl;

	UPROPERTY()
	FString RegionCode;

	UFUNCTION()
	void RetrieveNewTokens();

	UFUNCTION()
	void GetResponseTime();

	void OnRetrieveNewTokensResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void OnGetResponseTimeResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void OnStartMatchmakingResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	
	void OnStopMatchmakingResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
		
};
