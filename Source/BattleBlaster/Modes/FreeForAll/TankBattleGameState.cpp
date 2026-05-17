// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/FreeForAll/TankBattleGameState.h"

ATankBattleGameState::ATankBattleGameState()
{
	WinnerIndex = -1;
	TargetScore = 7;
	PlayerCount = 2;
	GameOverDelay = 3.0f;
}

void ATankBattleGameState::ResetForNewGame()
{
	Super::ResetForNewGame();
	WinnerIndex = -1;
	TargetScore = 7;
	PlayerCount = 2;
	GameOverDelay = 3.0f;
	PlayerScores.Empty();
}

void ATankBattleGameState::InitializePlayerData(int32 InPlayerCount, int32 InTargetScore)
{
	PlayerCount = InPlayerCount;
	TargetScore = InTargetScore;
	PlayerScores.Init(0, InPlayerCount);
}

void ATankBattleGameState::AddPlayerScore(int32 SlotId, int32 Amount)
{
	if (PlayerScores.IsValidIndex(SlotId))
	{
		PlayerScores[SlotId] += Amount;
	}
}

int32 ATankBattleGameState::GetPlayerScore(int32 SlotId) const
{
	if (PlayerScores.IsValidIndex(SlotId))
	{
		return PlayerScores[SlotId];
	}
	return 0;
}
