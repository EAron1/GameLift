// Fill out your copyright notice in the Description page of Project Settings.


#include "GameLiftGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "TextReaderComponent.h"

UGameLiftGameInstance::UGameLiftGameInstance()
{
	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));

	ApiUrl = TextReader->ReadFile("Urls/ApiUrl.txt");
	RegionCode = TextReader->ReadFile("Urls/RegionCode.txt");

	HttpModule = &FHttpModule::Get();

}

void UGameLiftGameInstance::Shutdown()
{
	Super::Shutdown();

	if (AccessToken.Len() > 0)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> InvalidateTokensRequest = HttpModule->CreateRequest();
		InvalidateTokensRequest->SetURL(ApiUrl + "/invalidatetokens");
		InvalidateTokensRequest->SetVerb("GET");
		InvalidateTokensRequest->SetHeader("Content-Type", "application/json");
		InvalidateTokensRequest->SetHeader("Authorization", AccessToken);
		InvalidateTokensRequest->ProcessRequest();
	}
}

void UGameLiftGameInstance::Init()
{
	Super::Init();
	
	GetWorld()->GetTimerManager().SetTimer(GetResponseTimeHandle, this, &UGameLiftGameInstance::GetResponseTime, 1.f, true, 1.f);
}

void UGameLiftGameInstance::SetCognitoTokens(FString inAccessToken, FString inIdToken, FString inRefreshToken)
{
	AccessToken = inAccessToken;
	IdToken = inIdToken;
	RefreshToken = inRefreshToken;

	//UE_LOG(LogTemp, Warning, TEXT("access token: %s"), *AccessToken);
	//UE_LOG(LogTemp, Warning, TEXT("refresh token: %s"), *RefreshToken);

	GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftGameInstance::RetrieveNewTokens, 1.f, false, 3300.f);
}

void UGameLiftGameInstance::RetrieveNewTokens()
{
	if (AccessToken.Len() > 0 && RefreshToken.Len() > 0)
	{
		TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
		RequestObj->SetStringField("refreshToken", RefreshToken);

		FString RequestBody;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

		if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer))
		{
			TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RetriveNewTokensRequest = HttpModule->CreateRequest();
			RetriveNewTokensRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftGameInstance::OnRetrieveNewTokensResponseRecieved);
			RetriveNewTokensRequest->SetURL(ApiUrl + "/retrievenewtokens");
			RetriveNewTokensRequest->SetVerb("POST");
			RetriveNewTokensRequest->SetHeader("Content-Type", "application/json");
			RetriveNewTokensRequest->SetHeader("Authorization", AccessToken);
			RetriveNewTokensRequest->SetContentAsString(RequestBody);
			RetriveNewTokensRequest->ProcessRequest();
		}
		else
		{
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftGameInstance::RetrieveNewTokens, 1.f, false, 30.f);
		}
	}
}

void UGameLiftGameInstance::GetResponseTime()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> GetResponseTimeRequest = HttpModule->CreateRequest();
	GetResponseTimeRequest->OnProcessRequestComplete().BindUObject(this, &UGameLiftGameInstance::OnGetResponseTimeResponseRecieved);
	GetResponseTimeRequest->SetURL("https://gamelift." + RegionCode + ".amazonaws.com");
	GetResponseTimeRequest->SetVerb("GET");
	GetResponseTimeRequest->ProcessRequest();
}

void UGameLiftGameInstance::OnRetrieveNewTokensResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			if (JsonObject->HasField("accessToken") && JsonObject->HasField("idToken"))
			{
				SetCognitoTokens(JsonObject->GetStringField("accessToken"), JsonObject->GetStringField("idToken"), RefreshToken);
			}
		}
		else
		{
			GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftGameInstance::RetrieveNewTokens, 1.f, false, 30.f);
		}
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(RetrieveNewTokensHandle, this, &UGameLiftGameInstance::RetrieveNewTokens, 1.f, false, 30.f);
	}
}

void UGameLiftGameInstance::OnGetResponseTimeResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (PlayerLatencies.Num() >= 4)
	{
		PlayerLatencies.RemoveNode(PlayerLatencies.GetHead());
	}

	float ResponseTime = Request->GetElapsedTime() * 1000;

	//UE_LOG(LogTemp, Warning, TEXT("Response time in ms: %s"), *FString::SanitizeFloat(ResponseTime));

	PlayerLatencies.AddTail(ResponseTime);
}

void UGameLiftGameInstance::OnStartMatchmakingResponseRecieved(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	
}
