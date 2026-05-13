#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TankGameState.h"
#include "TeamBattleGameState.h"
#include "TankPlayerState.h"
#include "TeamBattlePlayerState.h"
#include "HUDWidget.h"
#include "Tank.h"
#include "ScreenMessage.h"
#include "ScoresDisplayWidget.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "TeamBattleGameOverWidget.h"
#include "TankBuffComponent.h"
#include "BuffTypes.h"
#include "TeamBattleGameMode.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BATTLEBLASTER_API ATeamBattleGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATeamBattleGameMode();

	// ================= 配置参数 =================

	// 游戏结束延迟时间（秒）
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float GameOverDelay = 3.0f;

	// 玩家死亡后的复活延迟时间
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float RespawnDelay = 2.0f;

	// 玩家复活无敌的时间
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	float InvincibleTime = 3.0f;

	// 复活时生命值百分比 (0.0-1.0)
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnHealthPercent = 0.5f;

	// 复活时弹药百分比 (0.0-1.0)
	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RespawnAmmoPercent = 0.5f;

	// 开场倒计时 ("3, 2, 1, GO")
	UPROPERTY(EditAnywhere, Category = "Game Rules")
	int32 CountdownDelay = 3;

	// 坦克蓝图类
	UPROPERTY(EditDefaultsOnly, Category = "Game Setup")
	TSubclassOf<class ATank> TankClass;

	// ================= 团队模式固定配置 =================

	// 团队模式固定4人
	static constexpr int32 TeamBattlePlayerCount = 4;

	// 阵营定义：0=红色，1=蓝色
	enum class ETeamCamp : uint8
	{
		Red = 0,
		Blue = 1
	};

	// ================= 运行时状态 =================

	// 从 GameInstance 读取的目标玩家数量
	int32 TargetPlayerCount = 4;
	// 为了四人分屏而使用的"视口玩家数量"
	int32 ViewportPlayerCount = 4;
	// 从 GameInstance 读取的获胜目标分数
	int32 TargetScore = 7;
	// 倒计时剩余秒数
	int32 CountdownSeconds;
	// 比赛进行的总时长
	int32 MatchTimeSeconds = 0;

	// 阵营分数：索引0=红色阵营，索引1=蓝色阵营
	TArray<int32> TeamScores;

	// 存储每个玩家死亡时的Buff信息，用于复活时恢复
	TArray<TArray<FActiveBuffUIInfo>> PlayerSavedBuffs;

	// 胜者阵营 (-1:进行中, 0:红色胜, 1:蓝色胜)
	int32 WinnerCampIndex = -1;

	// ================= 辅助函数 =================

	// 获取当前的 TeamBattleGameState
	ATeamBattleGameState* GetTeamBattleGameState() const;

	// ================= AI控制相关 =================
	// 实际连接的手柄数量
	int32 ConnectedGamepadCount = 1;
	// 记录每个玩家索引是否被AI控制 (true=AI, false=真实玩家)
	TArray<bool> bIsPlayerAIControlled;

	// --- 定时器句柄 ---
	FTimerHandle CountdownTimerHandle;
	FTimerHandle MatchTimerHandle;
	FTimerHandle ExtraViewportBlackTimerHandle;

	// 使用数组存储所有活跃的坦克实例
	UPROPERTY()
	TArray<ATank*> ActiveTanks;

	// 使用数组存储所有找到的出生点 (Tag: P0, P1, P2, P3)
	UPROPERTY()
	TArray<AActor*> PlayerStarts;

	// ================= 特效与UI =================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VictoryEffects")
	UNiagaraSystem* VictoryEffect;

	UPROPERTY(EditDefaultsOnly, Category = "RespawnEffects")
	UNiagaraSystem* RespawnNiagaraVFX;

	// 简单的屏幕消息 UI
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UScreenMessage> ScreenMessageClass;

	UPROPERTY()
	UScreenMessage* ScreenMessageWidget;

	// 比分板 UI
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UScoresDisplayWidget> ScoresWidgetClass;

	UPROPERTY()
	UScoresDisplayWidget* ScoresWidgetInstance;

	// 团队战斗结束结算界面
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UTeamBattleGameOverWidget> TeamBattleGameOverWidgetClass;

	UPROPERTY()
	UTeamBattleGameOverWidget* TeamBattleGameOverWidgetInstance;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> BlackoutWidgetClass;

	// 游戏结束或切换关卡时的清理函数
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ================= 阵营相关函数 =================

	// 根据玩家索引获取阵营
	ETeamCamp GetPlayerCamp(int32 PlayerIndex) const;

	// 获取指定阵营的玩家索引数组
	TArray<int32> GetPlayersInCamp(ETeamCamp Camp) const;

	// 检查两个玩家是否是同一阵营
	bool IsSameCamp(int32 PlayerIndexA, int32 PlayerIndexB) const;

	// 增加指定阵营的分数（由 PlayerState 的 HandleKillConfirmed 调用）
	void AddTeamScore(int32 CampIndex, int32 Amount);

	// 检查是否允许造成伤害（团队模式下禁止友军伤害）
	bool CanDealDamage(class AController* DamageCauser, class AActor* DamageVictim) const;

	// ================= 函数声明 =================
protected:
	virtual void BeginPlay() override;

public:
	// 坦克死亡事件处理（绑定到 ATank::OnKilled 委托）
	// KDA 已在 ATankPlayerState::ProcessDeath 内部完成，
	// 阵营积分已在 ATankPlayerState::HandleKillConfirmed → ATeamBattlePlayerState::HandleKillConfirmed
	//   → ATeamBattleGameMode::AddTeamScore 中完成
	// 本方法只处理：GameState 死亡数更新 / Buff 保存 / 复活计时 / 胜负判定
	UFUNCTION()
	void HandleTankKilled(class ATank* DeadTank, class ATank* KillerTank);

	// 复活指定索引的玩家
	void RespawnPlayer(int32 PlayerIndex);

	// 结束无敌状态的回调
	void EndInvincibility(ATank* Tank, UNiagaraComponent* SpawnedSystem);

	// 游戏结束回调
	void OnGameOverTimerTimeOut();

	// 弹出团队战斗结束结算界面
	void ShowTeamBattleGameOver();

	// 开场倒计时回调
	void OnCountdownTimerTimeout();

	// 比赛时间更新回调
	void UpdateMatchTime();

	// 将多余视口渲染为纯黑
	void ApplyBlackScreenToExtraViewports();

	// 更新阵营分数显示
	void UpdateTeamScoresDisplay();

private:
	// 用来保存所有黑屏UI的指针，方便后面清理它们
	UPROPERTY()
	TArray<UUserWidget*> BlackoutWidgetInstances;
};
