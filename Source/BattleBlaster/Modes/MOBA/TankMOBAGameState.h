// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/State/TankGameState.h"
#include "TankMOBAGameState.generated.h"

class ATurret;

/**
 * TankMOBAGameState - MOBA模式的游戏状态
 * 继承自TankGameState，管理防御塔、胜负判定等
 */
UCLASS()
class BATTLEBLASTER_API ATankMOBAGameState : public ATankGameState
{
	GENERATED_BODY()

public:
	ATankMOBAGameState();

	// ================= 防御塔管理 =================

	// 注册防御塔
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void RegisterTurret(ATurret* Turret);

	// 注销防御塔
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void UnregisterTurret(ATurret* Turret);

	// 获取所有阵营的存活主防御塔数量
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetAliveCoreTurretCount() const;

	// 获取指定阵营的存活主防御塔
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetAliveCoreTurretCountByCamp(int32 CampIndex) const;

	// 获取指定阵营的存活外防御塔数量
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetAliveOuterTurretCountByCamp(int32 CampIndex) const;

	// 检查指定阵营是否还有任何防御塔
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	bool HasAliveTowersByCamp(int32 CampIndex) const;

	// 获取存活阵营数量
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetAliveCampCount() const;

	// 获取存活阵营索引
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetAliveCampIndex() const;

	// 检查游戏是否结束（只剩一个阵营）
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	bool IsGameOver() const { return bIsGameOver; }

	// 设置游戏结束
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetGameOver(bool bOver) { bIsGameOver = bOver; }

	// 获取获胜阵营索引
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	int32 GetWinningCampIndex() const { return WinningCampIndex; }

	// 设置获胜阵营索引
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void SetWinningCampIndex(int32 Index) { WinningCampIndex = Index; }

	// 当防御塔被摧毁时调用
	UFUNCTION(BlueprintCallable, Category = "MOBA")
	void OnTurretDestroyed(ATurret* DestroyedTurret);

	// 检查游戏结束条件
	void CheckGameOverCondition();

	// 重置状态
	virtual void ResetForNewGame() override;

	// 各阵营存活的主防御塔数量
	UPROPERTY()
	TMap<int32, int32> CoreTurretCountByCamp;

	// 各阵营存活的外防御塔数量
	UPROPERTY()
	TMap<int32, int32> OuterTurretCountByCamp;

protected:
	// 所有防御塔列表
	UPROPERTY()
	TArray<ATurret*> AllTowers;

	// 游戏是否已结束
	UPROPERTY()
	bool bIsGameOver;

	// 获胜阵营索引
	UPROPERTY()
	int32 WinningCampIndex;
};
