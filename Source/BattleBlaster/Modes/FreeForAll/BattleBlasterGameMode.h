#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Shared/State/TankGameState.h"
#include "Modes/FreeForAll/TankBattleGameState.h"
#include "Shared/State/TankPlayerState.h"
#include "Modes/FreeForAll/TankBattlePlayerState.h"
#include "Shared/UI/HUDWidget.h"
#include "Shared/Pawns/Tank.h"
#include "Shared/UI/ScreenMessage.h"
#include "Shared/UI/ScoresDisplayWidget.h"
#include "NiagaraFunctionLibrary.h" 
#include "NiagaraComponent.h"       
#include "Modes/FreeForAll/UI/MultiBattleGameOverWidget.h"
#include "Shared/Buffs/TankBuffComponent.h"
#include "Shared/Buffs/BuffTypes.h"
#include "BattleBlasterGameMode.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class BATTLEBLASTER_API ABattleBlasterGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ABattleBlasterGameMode();

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


    // ================= 运行时状态 =================

    // 从 GameInstance 读取的目标玩家数量
    int32 TargetPlayerCount = 2;
    // 为了三人模式四宫格分屏而使用的“视口玩家数量”（3人时为4）
    int32 ViewportPlayerCount = 0;
    // 从 GameInstance 读取的获胜目标分数
    int32 TargetScore = 7;
    // 倒计时剩余秒数
    int32 CountdownSeconds;
    // 比赛进行的总时长
    int32 MatchTimeSeconds = 0;

    // 存储每个玩家死亡时的Buff信息，用于复活时恢复
    TArray<TArray<FActiveBuffUIInfo>> PlayerSavedBuffs;

    // 胜者索引 (-1:进行中, 0:P0胜, 1:P1胜...)
    int32 WinnerIndex = -1;

    // ================= 辅助函数 =================

    // 获取当前的 TankBattleGameState
    ATankBattleGameState* GetTankBattleGameState() const;

    // ================= AI控制相关 =================
    // 实际连接的手柄数量
    int32 ConnectedGamepadCount = 1;
    // 记录每个玩家索引是否被AI控制 (true=AI, false=真实玩家)
    UPROPERTY()
    TArray<bool> bIsPlayerAIControlled;

    // --- 定时器句柄 ---
    FTimerHandle CountdownTimerHandle;
    FTimerHandle MatchTimerHandle;
    // 额外视口（例如三人模式下的第4个视口）黑屏用定时器
    FTimerHandle ExtraViewportBlackTimerHandle;

    // 【修改】使用数组存储所有活跃的坦克实例
    UPROPERTY()
    TArray<ATank*> ActiveTanks;

    // 【修改】使用数组存储所有找到的出生点 (Tag: P0, P1, P2...)
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

    // 多人死斗结束结算界面
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UMultiBattleGameOverWidget> MultiBattleGameOverWidgetClass;

    UPROPERTY()
    UMultiBattleGameOverWidget* MultiBattleGameOverWidgetInstance;

    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UUserWidget> BlackoutWidgetClass;

    // 【新增】：游戏结束或切换关卡时的清理函数
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // 【新增】：用来保存所有黑屏UI的指针，方便后面清理它们
    UPROPERTY()
    TArray<UUserWidget*> BlackoutWidgetInstances;

    // ================= 函数声明 =================
protected:
    virtual void BeginPlay() override;

public:
    // 坦克死亡事件处理（绑定到 ATank::OnKilled 委托）
    // KDA 已在 ATankPlayerState::ProcessDeath 内部完成，
    // 本方法只处理：GameState 分数 / 复活 / 胜负判定 / UI 刷新
    UFUNCTION()
    void HandleTankKilled(class ATank* DeadTank, class ATank* KillerTank);

    // 复活指定索引的玩家
    void RespawnPlayer(int32 PlayerIndex);

    // 结束无敌状态的回调
    void EndInvincibility(ATank* Tank, UNiagaraComponent* SpawnedSystem);

    // 游戏结束回调
    void OnGameOverTimerTimeOut();

    // 弹出多人死斗结束结算界面
    void ShowMultiBattleGameOver();

    // 开场倒计时回调
    void OnCountdownTimerTimeout();

    // 比赛时间更新回调
    void UpdateMatchTime();

    // 将多余视口（如三人模式下的第4块）渲染为纯黑
    void ApplyBlackScreenToExtraViewports();
};