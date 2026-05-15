// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/MOBA/TankMOBAGameState.h"
#include "Modes/MOBA/Turret.h"
#include "Modes/MOBA/TankMOBAGameMode.h"

ATankMOBAGameState::ATankMOBAGameState()
{
	bIsGameOver = false;
	WinningCampIndex = -1;
}

void ATankMOBAGameState::RegisterTurret(ATurret* Turret)
{
	if (!Turret || AllTowers.Contains(Turret))
	{
		return;
	}

	AllTowers.Add(Turret);

	int32 CampIndex = Turret->CampIndex;
	
	// 根据塔的类型增加计数
	// 这里暂时不做区分，由外部调用 OnTurretDestroyed 时处理
}

void ATankMOBAGameState::UnregisterTurret(ATurret* Turret)
{
	if (!Turret)
	{
		return;
	}

	AllTowers.Remove(Turret);
}

void ATankMOBAGameState::OnTurretDestroyed(ATurret* DestroyedTurret)
{
	if (!DestroyedTurret)
	{
		return;
	}

	int32 CampIndex = DestroyedTurret->CampIndex;
	bool bIsCoreTurret = DestroyedTurret->bIsCoreTurret;

	// 减少对应防御塔计数
	if (bIsCoreTurret)
	{
		if (CoreTurretCountByCamp.Contains(CampIndex))
		{
			CoreTurretCountByCamp[CampIndex] = FMath::Max(0, CoreTurretCountByCamp[CampIndex] - 1);
		}

		// 直接通知 GameMode 隐藏 UI
		if (ATankMOBAGameMode* MOBAGameMode = Cast<ATankMOBAGameMode>(GetWorld()->GetAuthGameMode()))
		{
			MOBAGameMode->HideCoreTurretImage(CampIndex);
		}
	}
	else
	{
		if (OuterTurretCountByCamp.Contains(CampIndex))
		{
			OuterTurretCountByCamp[CampIndex] = FMath::Max(0, OuterTurretCountByCamp[CampIndex] - 1);
		}
	}

	// 【移除】：游戏结束判定现在只在玩家被淘汰时进行（由 GameMode::CheckGameOver 处理）
	// 不再在防御塔被摧毁时直接判定游戏结束，以支持"核心塔被摧毁后玩家仍可复活"的机制
}

int32 ATankMOBAGameState::GetAliveCoreTurretCount() const
{
	int32 Total = 0;
	for (const auto& Pair : CoreTurretCountByCamp)
	{
		Total += Pair.Value;
	}
	return Total;
}

int32 ATankMOBAGameState::GetAliveCoreTurretCountByCamp(int32 CampIndex) const
{
	if (CoreTurretCountByCamp.Contains(CampIndex))
	{
		return CoreTurretCountByCamp[CampIndex];
	}
	return 0;
}

int32 ATankMOBAGameState::GetAliveOuterTurretCountByCamp(int32 CampIndex) const
{
	if (OuterTurretCountByCamp.Contains(CampIndex))
	{
		return OuterTurretCountByCamp[CampIndex];
	}
	return 0;
}

bool ATankMOBAGameState::HasAliveTowersByCamp(int32 CampIndex) const
{
	int32 CoreCount = GetAliveCoreTurretCountByCamp(CampIndex);
	int32 OuterCount = GetAliveOuterTurretCountByCamp(CampIndex);
	return (CoreCount + OuterCount) > 0;
}

int32 ATankMOBAGameState::GetAliveCampCount() const
{
	int32 Count = 0;
	TSet<int32> UniqueCamps;
	
	for (const auto& Pair : CoreTurretCountByCamp)
	{
		if (Pair.Value > 0)
		{
			UniqueCamps.Add(Pair.Key);
		}
	}
	
	for (const auto& Pair : OuterTurretCountByCamp)
	{
		if (Pair.Value > 0)
		{
			UniqueCamps.Add(Pair.Key);
		}
	}
	
	return UniqueCamps.Num();
}

int32 ATankMOBAGameState::GetAliveCampIndex() const
{
	// 遍历找到唯一的存活阵营
	TSet<int32> AliveCamps;
	
	for (const auto& Pair : CoreTurretCountByCamp)
	{
		if (Pair.Value > 0)
		{
			AliveCamps.Add(Pair.Key);
		}
	}
	
	for (const auto& Pair : OuterTurretCountByCamp)
	{
		if (Pair.Value > 0)
		{
			AliveCamps.Add(Pair.Key);
		}
	}
	
	if (AliveCamps.Num() == 1)
	{
		for (int32 Camp : AliveCamps)
		{
			return Camp;
		}
	}
	
	return -1;
}

void ATankMOBAGameState::CheckGameOverCondition()
{
	int32 AliveCamps = GetAliveCampCount();
	
	// 只剩一个阵营时游戏结束
	if (AliveCamps <= 1 && !bIsGameOver)
	{
		bIsGameOver = true;
		WinningCampIndex = GetAliveCampIndex();
		GameStatus = EGameStatus::Ended;
		
		UE_LOG(LogTemp, Display, TEXT("MOBA Game Over! Winner Camp: %d"), WinningCampIndex);
	}
}

void ATankMOBAGameState::ResetForNewGame()
{
	Super::ResetForNewGame();

	AllTowers.Empty();
	CoreTurretCountByCamp.Empty();
	OuterTurretCountByCamp.Empty();
	bIsGameOver = false;
	WinningCampIndex = -1;
}
