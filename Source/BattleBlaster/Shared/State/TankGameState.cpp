// Fill out your copyright notice in the Description page of Project Settings.

#include "Shared/State/TankGameState.h"

ATankGameState::ATankGameState()
{
	MatchTimeSeconds = 0;
	CountdownSeconds = 0;
	GameStatus = EGameStatus::Waiting;
}

void ATankGameState::ResetForNewGame()
{
	MatchTimeSeconds = 0;
	CountdownSeconds = 0;
	GameStatus = EGameStatus::Waiting;
}
