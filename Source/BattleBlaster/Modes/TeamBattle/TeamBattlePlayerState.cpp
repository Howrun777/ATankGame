// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/TeamBattle/TeamBattlePlayerState.h"
#include "Modes/TeamBattle/TeamBattleGameMode.h"
#include "Shared/Pawns/Tank.h"

ATeamBattlePlayerState::ATeamBattlePlayerState()
{
	TeamCamp = 0;
	TeamScoreContribution = 0;
	bIsInvincible = false;
	RespawnTime = 0.0f;
}

void ATeamBattlePlayerState::ResetForNewGame()
{
	Super::ResetForNewGame();
	TeamCamp = 0;
	TeamScoreContribution = 0;
	bIsInvincible = false;
	RespawnTime = 0.0f;
}

void ATeamBattlePlayerState::HandleKillConfirmed(ATank* Victim)
{
	if (!Victim) return;


	// 获取受害者的阵营
	int32 VictimCampIndex = -1;
	ATeamBattlePlayerState* VictimPS = Cast<ATeamBattlePlayerState>(Victim->GetPlayerState());
	if (VictimPS)
	{
		VictimCampIndex = static_cast<int32>(VictimPS->TeamCamp);
	}

	// 只有跨阵营击杀才加分
	if (static_cast<int32>(TeamCamp) != VictimCampIndex)
	{
		TeamScoreContribution++;
	}
}
