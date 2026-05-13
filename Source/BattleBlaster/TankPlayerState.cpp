// Fill out your copyright notice in the Description page of Project Settings.

#include "TankPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Tank.h"
#include "TankPlayerController.h" // forward declared in .h; included here to avoid circular dep issues

ATankPlayerState::ATankPlayerState()
{
	PlayerIndex = -1;
	TeamID = 0;
	KillCount = 0;
	DeathCount = 0;
	AssistCount = 0;
	CurrentAmmo = 0;
	IsAlive = true;
	bHasSpawnLocation = false;
	HomeSpawnLocation = FVector::ZeroVector;
	HomeSpawnRotation = FRotator::ZeroRotator;

	// 开启 Tick 功能
	PrimaryActorTick.bCanEverTick = true;
	// 优化：没必要每帧清理，每 1 秒执行一次 Tick 就足够了，极其省性能！
	PrimaryActorTick.TickInterval = 1.0f;
}

void ATankPlayerState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每秒自动清理过期的仇人记录
	CleanUpExpiredAttackers();
}

void ATankPlayerState::RecordAttacker(ATank* Attacker)
{
	// 忽略无效攻击者，或者自己炸自己的情况
	if (!Attacker || Attacker == GetPawn()) return;

	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 1. 查找这个仇人是否已经在队列中了
	for (int32 i = 0; i < AttackerQueue.Num(); ++i)
	{
		if (AttackerQueue[i].AttackerTank == Attacker)
		{
			// 如果已经在队列中，先把它从老位置拔出来
			AttackerQueue.RemoveAt(i);
			break;
		}
	}

	// 2. 将最新的攻击记录插入到【队头 (Index 0)】
	FAttackerRecord NewRecord;
	NewRecord.AttackerTank = Attacker;
	NewRecord.LastAttackTime = CurrentTime;

	AttackerQueue.Insert(NewRecord, 0);
}

void ATankPlayerState::CleanUpExpiredAttackers()
{
	float CurrentTime = GetWorld()->GetTimeSeconds();

	// 【核心安全机制】：必须从后往前遍历删除，否则 TArray 会发生索引越界
	for (int32 i = AttackerQueue.Num() - 1; i >= 0; --i)
	{
		// 淘汰条件：1. 仇人断线/被销毁变成了空指针  2. 距离上次挨打超过了 7 秒
		if (!AttackerQueue[i].AttackerTank.IsValid() || (CurrentTime - AttackerQueue[i].LastAttackTime > 7.0f))
		{
			AttackerQueue.RemoveAt(i);
		}
	}
}

ATank* ATankPlayerState::ProcessDeath()
{
	// 1. 增加自己的死亡数
	DeathCount++;

	// 2. 结算前强制清理一次过期仇人
	CleanUpExpiredAttackers();

	ATank* KillerTank = nullptr;

	// 3. 遍历仇人队列结算 KDA
	for (int32 i = 0; i < AttackerQueue.Num(); ++i)
	{
		if (!AttackerQueue[i].AttackerTank.IsValid()) continue;
		ATank* Attacker = AttackerQueue[i].AttackerTank.Get();
		if (!Attacker) continue;

		// 这里获取击杀者/助攻者的 PlayerState（如果是 AI，必须保证 AIController 开启了 bWantsPlayerState）
		if (ATankPlayerState* AttackerPS = Attacker->GetPlayerState<ATankPlayerState>())
		{
			if (i == 0) // 队头是最后一次伤害来源，算作凶手
			{
				KillerTank = Attacker;
				AttackerPS->KillCount++; // 加击杀

				if (ATank* DeadTank = Cast<ATank>(GetPawn()))
				{
					AttackerPS->HandleKillConfirmed(DeadTank);
				}
				KillerTank->HandleKillReward();
			}
			else // 其他人都算助攻
			{
				AttackerPS->AssistCount++; // 加助攻
			}
		}
	}

	// 4. 刷新受影响的真人玩家屏幕上的 KDA UI
	RefreshKDAUI();

	// 5. 广播死亡事件给 GameMode（GameMode 只用管给谁加积分，不用管 KDA 了）
	if (ATank* DeadTank = Cast<ATank>(GetPawn()))
	{
		DeadTank->OnKilled.Broadcast(DeadTank, KillerTank);
	}

	// 6. 清空仇恨记录准备复活
	AttackerQueue.Empty();

	return KillerTank;
}

void ATankPlayerState::RefreshKDAUI()
{
	// 刷新死者（自己）的 UI，如果是真人玩家的话
	if (ATank* DeadTank = Cast<ATank>(GetPawn()))
	{
		if (ATankPlayerController* PC = Cast<ATankPlayerController>(DeadTank->GetController()))
		{
			PC->UpdateKDA();
		}
	}

	// 刷新所有参与攻击的人的 UI
	for (const FAttackerRecord& Record : AttackerQueue)
	{
		if (!Record.AttackerTank.IsValid()) continue;
		ATank* AttackerTank = Record.AttackerTank.Get();
		if (!AttackerTank) continue;

		// 检查攻击者是不是真人（AIBotPlayerController 强转 ATankPlayerController 会失败，正好跳过 AI，非常安全）
		if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(AttackerTank->GetController()))
		{
			TankPC->UpdateKDA(); // 真人才需要刷新屏幕上的数字
		}
	}
}

void ATankPlayerState::HandleKillConfirmed(ATank* Victim)
{
	// 基类默认什么都不做
	// 各模式 PlayerState 子类重写此方法来实现自己的阵营加分等逻辑
}

void ATankPlayerState::SaveCurrentBuffs(const TArray<FActiveBuffUIInfo>& Buffs)
{
	CurrentBuffs = Buffs;
}

void ATankPlayerState::ResetForNewGame()
{
	PlayerIndex = 0;
	TeamID = 0;
	KillCount = 0;
	DeathCount = 0;
	AssistCount = 0;
	CurrentAmmo = 0;
	IsAlive = true;
	CurrentBuffs.Empty();
	bHasSpawnLocation = false;
	HomeSpawnLocation = FVector::ZeroVector;
	HomeSpawnRotation = FRotator::ZeroRotator;

	// 重置新游戏时也清空仇恨队列
	AttackerQueue.Empty();
}