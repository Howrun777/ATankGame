// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/Stage/TankStageGameState.h"

ATankStageGameState::ATankStageGameState()
{
	CurrentStageId = 1;
	RemainingTowerCount = 0;
	CurrentWave = 0;
	TotalWaves = 1;
	bIsVictory = false;
	GameOverDelay = 3.0f;
	InitialLives = 3;
}

void ATankStageGameState::ResetForNewGame()
{
	Super::ResetForNewGame();
	CurrentStageId = 1;
	RemainingTowerCount = 0;
	CurrentWave = 0;
	TotalWaves = 1;
	bIsVictory = false;
	GameOverDelay = 3.0f;
	InitialLives = 3;
}
