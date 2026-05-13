// Fill out your copyright notice in the Description page of Project Settings.

#include "TankStagePlayerState.h"

ATankStagePlayerState::ATankStagePlayerState()
{
	RemainingLives = 3;
	StageScore = 0;
	EnemyKillCount = 0;
	StageTime = 0.0f;
	bIsInvincible = false;
	CurrentStageId = 1;
}

void ATankStagePlayerState::ResetForNewGame()
{
	Super::ResetForNewGame();
	RemainingLives = 3;
	StageScore = 0;
	EnemyKillCount = 0;
	StageTime = 0.0f;
	bIsInvincible = false;
	CurrentStageId = 1;
}

void ATankStagePlayerState::HandleKillConfirmed(ATank* Victim)
{
	// PVE 模式：敌人是 Tower/AI，不存在玩家人头奖励，无需阵营计分
	// Victim 参数在本模式中被忽略
}
