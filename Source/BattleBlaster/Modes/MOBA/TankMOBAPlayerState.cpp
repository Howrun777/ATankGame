// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/MOBA/TankMOBAPlayerState.h"
#include "Net/UnrealNetwork.h"

ATankMOBAPlayerState::ATankMOBAPlayerState()
{
	CampIndex = -1;
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

void ATankMOBAPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATankMOBAPlayerState, CampIndex);
	DOREPLIFETIME(ATankMOBAPlayerState, bIsDead);
	DOREPLIFETIME(ATankMOBAPlayerState, bIsEliminated);
	DOREPLIFETIME(ATankMOBAPlayerState, RespawnTimeRemaining);
	DOREPLIFETIME(ATankMOBAPlayerState, CurrentRespawnDelay);
	DOREPLIFETIME(ATankMOBAPlayerState, bIsWaitingForRespawn);
	DOREPLIFETIME(ATankMOBAPlayerState, TurretDestroyedCount);
}

FLinearColor ATankMOBAPlayerState::GetCampColor() const
{
	switch (CampIndex)
	{
		case 0: return FLinearColor(1.0f, 0.0f, 0.0f); // 红色
		case 1: return FLinearColor(0.0f, 0.0f, 1.0f); // 蓝色
		case 2: return FLinearColor(0.0f, 1.0f, 0.0f); // 绿色
		case 3: return FLinearColor(1.0f, 1.0f, 0.0f); // 黄色
		default: return FLinearColor::White;
	}
}

void ATankMOBAPlayerState::ResetForNewGame()
{
	Super::ResetForNewGame();

	CampIndex = -1;
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

void ATankMOBAPlayerState::InitializeMOBAState(int32 InCampIndex)
{
	CampIndex = InCampIndex;
	SetSlotId(InCampIndex);
	SetTeamId(InCampIndex);
	bIsDead = false;
	bIsEliminated = false;
	RespawnTimeRemaining = 0.0f;
	CurrentRespawnDelay = 2.0f;
	bIsWaitingForRespawn = false;
	TurretDestroyedCount = 0;
}

float ATankMOBAPlayerState::CalculateRespawnDelay(float GameTime, float InitialDelay, float MaxDelay, float GrowthInterval, float GrowthAmount) const
{
	// 每隔 GrowthInterval 秒，复活延迟增长 GrowthAmount 秒
	float GrowthSteps = (GrowthInterval > 0.0f) ? FMath::FloorToFloat(GameTime / GrowthInterval) : 0.0f;
	float NewDelay = InitialDelay + GrowthSteps * GrowthAmount;
	return FMath::Min(NewDelay, MaxDelay);
}

void ATankMOBAPlayerState::HandleKillConfirmed(ATank* Victim)
{
	// MOBA 模式：每个玩家独立阵营（CampIndex = SlotId），
	// 击杀跨阵营玩家不涉及"阵营积分"概念（无团队分数），
	// 所有 KDA 增减已由基类 ProcessDeath 处理，这里不需要额外逻辑。
}
