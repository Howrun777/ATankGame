// Fill out your copyright notice in the Description page of Project Settings.

#include "TeamBattleGameState.h"

ATeamBattleGameState::ATeamBattleGameState()
{
	TeamScores.SetNum(2);
	TeamScores[0] = 0;  // 红队
	TeamScores[1] = 0;  // 蓝队
	WinnerCampIndex = -1;
	TargetScore = 7;
	PlayerCount = 4;
	GameOverDelay = 3.0f;
}

void ATeamBattleGameState::AddTeamScore(ETeamCamp Camp, int32 Points)
{
	uint8 CampIndex = static_cast<uint8>(Camp);
	if (TeamScores.IsValidIndex(CampIndex))
	{
		TeamScores[CampIndex] += Points;
	}
}

int32 ATeamBattleGameState::GetTeamScore(ETeamCamp Camp) const
{
	uint8 CampIndex = static_cast<uint8>(Camp);
	if (TeamScores.IsValidIndex(CampIndex))
	{
		return TeamScores[CampIndex];
	}
	return 0;
}

void ATeamBattleGameState::ResetForNewGame()
{
	Super::ResetForNewGame();
	TeamScores.SetNum(2);
	TeamScores[0] = 0;
	TeamScores[1] = 0;
	WinnerCampIndex = -1;
	TargetScore = 7;
	PlayerCount = 4;
	GameOverDelay = 3.0f;
}

void ATeamBattleGameState::InitializePlayerData(int32 InPlayerCount)
{
	PlayerCount = InPlayerCount;
}
