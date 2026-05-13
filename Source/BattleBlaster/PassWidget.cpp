#include "PassWidget.h"
#include "BattleBlasterGameInstance.h"
#include "TankStageGameMode.h"
#include "Kismet/GameplayStatics.h"

void UPassWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshTimer = 0.0f;
	RefreshDisplay();
}

void UPassWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTimer += InDeltaTime;
	if (RefreshTimer >= 0.25f)
	{
		RefreshTimer = 0.0f;
		RefreshDisplay();
	}
}

void UPassWidget::RefreshHearts(int32 MaxLives, int32 CurrentLives)
{
	TArray<UImage*> Hearts;
	if (HeartImage_0) Hearts.Add(HeartImage_0);
	if (HeartImage_1) Hearts.Add(HeartImage_1);
	if (HeartImage_2) Hearts.Add(HeartImage_2);


	if (Hearts.Num() == 0 || !FullHeartTexture || !BrokenHeartTexture) return;

	for (int32 i = 0; i < Hearts.Num(); i++)
	{
		if (i < MaxLives)
		{
			Hearts[i]->SetVisibility(ESlateVisibility::Visible);
			// 前 CurrentLives 颗为满爱心，其余为破碎
			if (i < CurrentLives)
				Hearts[i]->SetBrushFromTexture(FullHeartTexture);
			else
				Hearts[i]->SetBrushFromTexture(BrokenHeartTexture);
		}
		else
		{
			Hearts[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPassWidget::RefreshDisplay()
{
	UBattleBlasterGameInstance* GI = Cast<UBattleBlasterGameInstance>(GetGameInstance());
	if (!GI) return;

	if (Text_CurrentPass)
		Text_CurrentPass->SetText(FText::AsNumber(GI->CurrentLevelIndex));

	if (Text_Record)
		Text_Record->SetText(FText::AsNumber(GI->BestLevelRecord));

	// ================== 【新增：刷新总游戏时间】 ==================
	if (Text_GameTime)
	{
		UWorld* World = GetWorld();
		const float GameTime = GI->GetCampaignTotalTime(World);

		// 调用格式化函数，并设置给 UI
		Text_GameTime->SetText(FText::FromString(FormatGameTime(GameTime)));
	}
	// ==============================================================

	// 爱心：仅在单人闯关模式下更新
	ATankStageGameMode* SPGM = Cast<ATankStageGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (SPGM)
	{
		int32 MaxLives = SPGM->GetMaxDeathCount();
		int32 CurrentLives = SPGM->GetRemainingLives();

		// 关掉烦人的 Log 刷屏
		// UE_LOG(LogTemp, VeryVerbose, TEXT("PassWidget: MaxLives=%d, CurrentLives=%d"), MaxLives, CurrentLives);

		RefreshHearts(MaxLives, CurrentLives);
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("PassWidget: TankStageGameMode not found!"));
	}
}

FString UPassWidget::FormatGameTime(float TotalSeconds) const
{
	int32 Total = FMath::Max(0, FMath::FloorToInt(TotalSeconds));
	int32 Hours = Total / 3600;
	int32 Minutes = (Total % 3600) / 60;
	int32 Seconds = Total % 60;
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}