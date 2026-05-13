// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TankGameState.generated.h"

// 游戏状态枚举（需要在类外部定义）
UENUM(BlueprintType)
enum class EGameStatus : uint8
{
	Waiting   UMETA(DisplayName = "Waiting"),
	Countdown UMETA(DisplayName = "Countdown"),
	Playing   UMETA(DisplayName = "Playing"),
	Ended     UMETA(DisplayName = "Ended")
};

/**
 * TankGameState 基类 - 所有Tank游戏模式共用的游戏状态
 */
UCLASS()
class BATTLEBLASTER_API ATankGameState : public AGameState
{
	GENERATED_BODY()

public:
	ATankGameState();

	// ================= 时间相关 =================

	// 比赛已进行时间（秒）
	UPROPERTY(VisibleAnywhere, Category = "Time")
	int32 MatchTimeSeconds;

	// 开场倒计时剩余秒数
	UPROPERTY(VisibleAnywhere, Category = "Time")
	int32 CountdownSeconds;

	// ================= 游戏状态 =================

	// 当前游戏状态
	UPROPERTY(VisibleAnywhere, Category = "Game Status")
	EGameStatus GameStatus;

	// ================= 通用函数 =================

	// 获取游戏状态
	EGameStatus GetGameStatus() const { return GameStatus; }

	// 设置游戏状态
	void SetGameStatus(EGameStatus Status) { GameStatus = Status; }

	// 获取比赛时间
	int32 GetMatchTime() const { return MatchTimeSeconds; }

	// 获取倒计时
	int32 GetCountdown() const { return CountdownSeconds; }

	// 是否游戏正在进行
	bool IsPlaying() const { return GameStatus == EGameStatus::Playing; }

	// 是否游戏已结束
	bool IsEnded() const { return GameStatus == EGameStatus::Ended; }

	// 重置状态（用于新游戏）
	virtual void ResetForNewGame();
};
