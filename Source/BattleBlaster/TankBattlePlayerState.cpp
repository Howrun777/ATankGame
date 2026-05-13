// Fill out your copyright notice in the Description page of Project Settings.

#include "TankBattlePlayerState.h"

ATankBattlePlayerState::ATankBattlePlayerState()
{
	bIsInvincible = false;
	RespawnTime = 0.0f;
}

void ATankBattlePlayerState::ResetForNewGame()
{
	Super::ResetForNewGame();
	bIsInvincible = false;
	RespawnTime = 0.0f;
}
