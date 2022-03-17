// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "MainMenuWidget.generated.h"

/**
 * 
 */

class UWebBrowser;
class UButton;
class UTextBlock;

UCLASS()
class GAMELIFT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	FTimerHandle SetAveragePlayerLatencyHandle;

protected:

	void NativeConstruct() override;
	
private:

	FHttpModule* HttpModule;

	UPROPERTY()
	FString LoginUrl;

	UPROPERTY()
	FString ApiUrl;

	UPROPERTY()
	FString CallbackUrl;
	
	UPROPERTY()
	FString RegionCode;

	UPROPERTY()
	UWebBrowser* WebBrowser;

	UPROPERTY(meta = (BindWidget))
	UButton* Button_Matchmaking;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Wins;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Loses;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_Ping;
	
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TextBlock_MatchmakingEvent;

	UPROPERTY()
	float AveragePlayerLatency;

	UPROPERTY()
	bool SearchingForGame;

	UFUNCTION()
	void HandleLoginUrlChange();

	UFUNCTION()
	void SetAveragePlayerLatency();

	UFUNCTION()
	void OnMatchmakingButtonClicked();

	void OnExchangeCodeForTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

};
