// Fill out your copyright notice in the Description page of Project Settings.


#include "MainMenuWidget.h"
#include "WebBrowser.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserCookieManager.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#include "TextReaderComponent.h"
#include "GameLiftGameInstance.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));

	LoginUrl = TextReader->ReadFile("Urls/LoginUrl.txt");
	ApiUrl = TextReader->ReadFile("Urls/ApiUrl.txt");
	CallbackUrl = TextReader->ReadFile("Urls/CallbackUrl.txt");
	RegionCode = TextReader->ReadFile("Urls/RegionCode.txt");

	AveragePlayerLatency = 60.f;
	SearchingForGame = false;

	HttpModule = &FHttpModule::Get();
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	bIsFocusable = true;

	WebBrowser = (UWebBrowser*)GetWidgetFromName(TEXT("WebBrowser_Login"));

	FScriptDelegate MatchmakingDelagate;
	MatchmakingDelagate.BindUFunction(this, "OnMatchmakingButtonClicked");
	Button_Matchmaking->OnClicked.Add(MatchmakingDelagate);

	GetWorld()->GetTimerManager().SetTimer(SetAveragePlayerLatencyHandle, this, &UMainMenuWidget::SetAveragePlayerLatency, 1.f, true);

	IWebBrowserSingleton* WebBrowserSingleton = IWebBrowserModule::Get().GetSingleton();
	if (WebBrowserSingleton)
	{
		TOptional<FString> DefaultContext;
		TSharedPtr<IWebBrowserCookieManager> CookieManager = WebBrowserSingleton->GetCookieManager(DefaultContext);
		if (CookieManager)
		{
			CookieManager->DeleteCookies();
		}

	}

	WebBrowser->LoadURL(LoginUrl);

	FScriptDelegate LoginDelagate;
	LoginDelagate.BindUFunction(this, "HandleLoginUrlChange");
	WebBrowser->OnUrlChanged.Add(LoginDelagate);
}

void UMainMenuWidget::HandleLoginUrlChange()
{
	FString BrowserUrl = WebBrowser->GetUrl();
	FString Url, QueryParameters;

	if (BrowserUrl.Split("?", &Url, &QueryParameters))
	{
		if (Url.Equals(CallbackUrl))
		{
			FString ParameterName, ParameterValue;
			if (QueryParameters.Split("=", &ParameterName, &ParameterValue))
			{
				if (ParameterName.Equals("code"))
				{
					ParameterValue = ParameterValue.Replace(*FString("#"), *FString(""));

					TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
					RequestObj->SetStringField(ParameterName, ParameterValue);

					FString RequestBody;
					TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

					if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer))
					{
						TSharedRef<IHttpRequest, ESPMode::ThreadSafe> ExchangeCodeForTokens = HttpModule->CreateRequest();
						ExchangeCodeForTokens->OnProcessRequestComplete().BindUObject(this, &UMainMenuWidget::OnExchangeCodeForTokensResponseReceived);
						ExchangeCodeForTokens->SetURL(ApiUrl + "/exchangecodefortokens");
						ExchangeCodeForTokens->SetVerb("POST");
						ExchangeCodeForTokens->SetHeader("Content-type", "application/json");
						ExchangeCodeForTokens->SetContentAsString(RequestBody);
						ExchangeCodeForTokens->ProcessRequest();
					}
				}
			}
		}
	}
}

void UMainMenuWidget::SetAveragePlayerLatency()
{
	UGameLiftGameInstance* GI = GetGameInstance<UGameLiftGameInstance>();
	if (GI)
	{
		float TotalPlayerLatency = 0.f;
		for (float playerLatency : GI->PlayerLatencies)
			TotalPlayerLatency += playerLatency;

		if (TotalPlayerLatency > 0 && GI->PlayerLatencies.Num() > 0)
		{
			AveragePlayerLatency = TotalPlayerLatency / GI->PlayerLatencies.Num();

			FString PingString = "Ping: " + FString::FromInt(FMath::RoundToInt(AveragePlayerLatency)) + "ms";
			TextBlock_Ping->SetText(FText::FromString(PingString));
		}
	}
}

void UMainMenuWidget::OnMatchmakingButtonClicked()
{
	Button_Matchmaking->SetIsEnabled(false);

	FString AccessToken;
	FString MatchmakingTicketId;

	UGameLiftGameInstance* GI = GetGameInstance<UGameLiftGameInstance>();
	if (GI)
	{
		AccessToken = GI->AccessToken;
		MatchmakingTicketId = GI->MatchmakingTicketId;
	}

	if (SearchingForGame)
	{
		SearchingForGame = false;

		if (AccessToken.Len() > 0 && MatchmakingTicketId.Len() > 0)
		{
			TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
			RequestObj->SetStringField("ticketId", MatchmakingTicketId);

			FString RequestBody;
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
			if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer))
			{
				TSharedRef<IHttpRequest> StopMatchmakingRequest = HttpModule->CreateRequest();
				StopMatchmakingRequest->OnProcessRequestComplete().BindUObject(this, &UMainMenuWidget::OnStopMatchmakingResponseReceived);
				StopMatchmakingRequest->SetURL(ApiUrl + "/stopmatchmaking");
				StopMatchmakingRequest->SetVerb("POST");
				StopMatchmakingRequest->SetHeader("Content-Type", "application/json");
				StopMatchmakingRequest->SetHeader("Authorization", AccessToken);
				StopMatchmakingRequest->SetContentAsString(RequestBody);
				StopMatchmakingRequest->ProcessRequest();
			}
		}
		
		UTextBlock* TextBlockButton = (UTextBlock*)Button_Matchmaking->GetChildAt(0);
		if(TextBlockButton)
			TextBlockButton->SetText(FText::FromString("Join Game"));

		TextBlock_MatchmakingEvent->SetText(FText::FromString(""));
	}
	else
	{
		SearchingForGame = true;

		UTextBlock* TextBlockButton = (UTextBlock*)Button_Matchmaking->GetChildAt(0);
		if (TextBlockButton)
			TextBlockButton->SetText(FText::FromString("Cancel Game"));

		TextBlock_MatchmakingEvent->SetText(FText::FromString("Searching for a match ..."));
	}
}

void UMainMenuWidget::OnExchangeCodeForTokensResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			if (JsonObject->HasField("access_token") && JsonObject->HasField("id_token") && JsonObject->HasField("refresh_token"))
			{
				UGameLiftGameInstance* GI = GetGameInstance<UGameLiftGameInstance>();
				if (GI)
				{
					FString AccessToken = JsonObject->GetStringField("access_token");
					GI->SetCognitoTokens(AccessToken, JsonObject->GetStringField("id_token"), JsonObject->GetStringField("refresh_token"));



					TSharedRef<IHttpRequest, ESPMode::ThreadSafe> GetPlayerDataRequest = HttpModule->CreateRequest();
					GetPlayerDataRequest->OnProcessRequestComplete().BindUObject(this, &UMainMenuWidget::OnGetPlayerDataResponseReceived);
					GetPlayerDataRequest->SetURL(ApiUrl + "/getplayerdata");
					GetPlayerDataRequest->SetVerb("GET");
					GetPlayerDataRequest->SetHeader("Authorization", AccessToken);
					GetPlayerDataRequest->ProcessRequest();
				}
			}
		}

	}
}

void UMainMenuWidget::OnGetPlayerDataResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>>  Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			if (JsonObject->HasField("playerData"))
			{
				TSharedPtr<FJsonObject> PlayerData = JsonObject->GetObjectField("playerData");
				TSharedPtr<FJsonObject> WinsObject = PlayerData->GetObjectField("Wins");
				TSharedPtr<FJsonObject> LossesObject = PlayerData->GetObjectField("Losses");

				FString Wins = WinsObject->GetStringField("N");
				FString Losses = LossesObject->GetStringField("N");

				TextBlock_Wins->SetText(FText::FromString("Wins: " + Wins));
				TextBlock_Loses->SetText(FText::FromString("Losses: " + Losses));

				WebBrowser->SetVisibility(ESlateVisibility::Hidden);
				Button_Matchmaking->SetVisibility(ESlateVisibility::Visible);
				TextBlock_Ping->SetVisibility(ESlateVisibility::Visible);
				TextBlock_MatchmakingEvent->SetVisibility(ESlateVisibility::Visible);
				TextBlock_Wins->SetVisibility(ESlateVisibility::Visible);
				TextBlock_Loses->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}
