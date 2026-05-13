// Fill out your copyright notice in the Description page of Project Settings.


#include "KDAWidget.h"
#include "Components/TextBlock.h"

void UKDAWidget::UpdateKDA(int32 Kills, int32 Deaths, int32 Assists)
{
	if (KillsText)
	{
		KillsText->SetText(FText::AsNumber(Kills));
	}

	if (DeathsText)
	{
		DeathsText->SetText(FText::AsNumber(Deaths));
	}

	if (AssistsText)
	{
		AssistsText->SetText(FText::AsNumber(Assists));
	}

	if (SummaryText)
	{
		const FString Summary = FString::Printf(TEXT("%d - %d - %d"), Kills, Deaths, Assists);
		SummaryText->SetText(FText::FromString(Summary));
	}
}

void UKDAWidget::SetColor(FLinearColor Color)
{
	if (KillsText)
	{
		KillsText->SetColorAndOpacity(Color);
	}

	if (DeathsText)
	{
		DeathsText->SetColorAndOpacity(Color);
	}

	if (AssistsText)
	{
		AssistsText->SetColorAndOpacity(Color);
	}

	if (SummaryText)
	{
		SummaryText->SetColorAndOpacity(Color);
	}
}
