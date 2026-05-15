// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Shared/Buffs/TankBuffComponent.h"
#include "TankPlayerState.generated.h"

class ATank; // 前向声明，避免头文件循环依赖

// ================= 仇人记录结构体 =================
USTRUCT()
struct FAttackerRecord
{
	GENERATED_BODY()

	// 使用弱指针，防止仇人被销毁或退出游戏后导致野指针崩溃
	TWeakObjectPtr<ATank> AttackerTank;

	// 记录最后一次受到该仇人攻击的时间
	float LastAttackTime;

	FAttackerRecord() : AttackerTank(nullptr), LastAttackTime(0.0f) {}
};


/**
 * TankPlayerState 基类 - 所有Tank游戏模式共用的玩家状态
 */
UCLASS()
class BATTLEBLASTER_API ATankPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATankPlayerState();

	virtual void Tick(float DeltaTime) override;

	// ================= 玩家索引 =================

	// 玩家索引 (0, 1, 2, 3)，用于关联GameMode中的数组
	UPROPERTY(VisibleAnywhere, Category = "Player Info")
	int32 PlayerIndex;

	// 队伍ID (如果有阵营机制，推荐加上这个属性)
	UPROPERTY(VisibleAnywhere, Category = "Player Info")
	int32 TeamID = 0;

	// ================= 玩家存活状态（跨 Pawn 保留）====================

	// 玩家是否存活（与 Tank Pawn 的 IsAlive 保持同步，但属于 PlayerState 层面）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player State")
	bool IsAlive = true;

	// ================= 出生点（跨 Pawn 保留）====================

	// 记录的出生点位置（复活/回城时使用）
	UPROPERTY(VisibleAnywhere, Category = "Player Info")
	FVector HomeSpawnLocation;

	// 记录的出生点旋转（复活/回城时使用）
	UPROPERTY(VisibleAnywhere, Category = "Player Info")
	FRotator HomeSpawnRotation;

	// 是否已记录出生点
	UPROPERTY(VisibleAnywhere, Category = "Player Info")
	bool bHasSpawnLocation = false;

	// ================= 弹药（跨 Pawn 保留）====================

	// 当前弹药数量（复活时恢复）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	int32 CurrentAmmo;

	// ================= 战斗统计 =================

	// 击杀数
	UPROPERTY(VisibleAnywhere, Category = "Combat Stats")
	int32 KillCount;

	// 死亡数
	UPROPERTY(VisibleAnywhere, Category = "Combat Stats")
	int32 DeathCount;

	// 助攻数
	UPROPERTY(VisibleAnywhere, Category = "Combat Stats")
	int32 AssistCount;

	// ================= Buff状态 =================

	// 当前拥有的Buff（死亡复活时恢复用）
	UPROPERTY(VisibleAnywhere, Category = "Buff")
	TArray<FActiveBuffUIInfo> CurrentBuffs;

	// ================= 仇人队列 =================
protected:
	// 存放 7 秒内攻击过我的所有仇人（队头为最后攻击者）
	TArray<FAttackerRecord> AttackerQueue;

public:
	// 记录受击（由 Tank 收到伤害时调用）
	void RecordAttacker(ATank* Attacker);

	// 处理死亡结算（由 Tank 死亡时调用）
	// 返回值：凶手 Tank 指针（用于 GameMode 的后续处理，如阵营积分/胜负判定）
	//         若仇人队列为空或凶手非 Tank，返回 nullptr
	ATank* ProcessDeath();

protected:
	// 子类可重写：凶手击杀确认通知（用于各模式自己的加分逻辑）
	// Victim = 被击杀的玩家自己（this）
	virtual void HandleKillConfirmed(ATank* Victim);

	// 清理超过 7 秒的过期记录
	void CleanUpExpiredAttackers();

	// 刷新所有受影响者的 KDA UI（死者 + 凶手 + 所有助攻者）
	// 凶手必定是 AttackerQueue[0]（队头是最后攻击者），无需额外参数
	void RefreshKDAUI();

	// ================= 通用函数 =================
public:
	// 获取仇人队列（用于 AI 威胁评估等）
	const TArray<FAttackerRecord>& GetAttackerQueue() const { return AttackerQueue; }

	// 添加击杀
	void AddKill() { KillCount++; }

	// 添加死亡
	void AddDeath() { DeathCount++; }

	// 添加助攻
	void AddAssist() { AssistCount++; }

	// 保存当前Buff
	void SaveCurrentBuffs(const TArray<FActiveBuffUIInfo>& Buffs);

	// 清除Buff
	void ClearBuffs() { CurrentBuffs.Empty(); }

	// 获取Buff列表
	const TArray<FActiveBuffUIInfo>& GetBuffs() const { return CurrentBuffs; }

	// 重置状态（用于新游戏）- 虚函数
	virtual void ResetForNewGame();

	// ================= 玩家状态同步（跨 Pawn 保留）=================
public:
	// 记录出生点（Tank 初始化时调用）
	void RecordSpawnLocation(const FVector& Location, const FRotator& Rotation)
	{
		HomeSpawnLocation = Location;
		HomeSpawnRotation = Rotation;
		bHasSpawnLocation = true;
	}

	// 更新弹药（Tank 开火时调用）
	void UpdateAmmo(int32 NewAmmo) { CurrentAmmo = NewAmmo; }

	// 设置存活状态
	void SetAlive(bool bAlive) { IsAlive = bAlive; }

	// 获取弹药
	int32 GetAmmo() const { return CurrentAmmo; }

	// 检查是否有出生点
	bool HasSpawnLocation() const { return bHasSpawnLocation; }

	// 获取出生点
	FVector GetHomeSpawnLocation() const { return HomeSpawnLocation; }
	FRotator GetHomeSpawnRotation() const { return HomeSpawnRotation; }
};
