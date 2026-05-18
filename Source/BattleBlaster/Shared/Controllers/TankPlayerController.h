#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Shared/UI/PauseMenuWidget.h"
#include "Shared/UI/HUDWidget.h"
#include "Modes/Stage/UI/PassWidget.h"
#include "InputActionValue.h" 
#include "InputMappingContext.h" 
#include "InputAction.h"
#include "TimerManager.h"
#include "TankPlayerController.generated.h"

// 前向声明
class UReturnToSpawnWidget;
class UHealthComponent;
class ATank;

class UBulletsWidget;
class UBuffListWidget;
class UPassWidget;
class UKDAWidget;

UCLASS()
class BATTLEBLASTER_API ATankPlayerController : public APlayerController
{
	GENERATED_BODY()

public:


	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;// 新增：游戏结束时清理UI


	//血量显示
	UPROPERTY(EditAnywhere)
	TSubclassOf<UHUDWidget> HUDWidgetClass;
	//血量显示
	UPROPERTY(VisibleAnywhere)
	UHUDWidget* HUDWidget;
	// 添加一个专门更新血条的辅助函数（让代码更干净）
	void UpdateHealthHUD(float HealthPercent, float ShieldPercent);

	// 提供给 Tank 调用的接口
	void SetHUDAmmo(int32 Current, int32 Max);

	UFUNCTION(Client, Reliable)
	void ClientSetHUDAmmo(int32 Current, int32 Max);

	// 在蓝图中设置我们要用的那个 WBP_AmmoHUD 类
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<class UUserWidget> AmmoWidgetClass;
	// 保存创建出来的 Widget 实例
	UPROPERTY()
	UBulletsWidget* AmmoWidget;

	// --- Buff列表UI代码 ---
	UPROPERTY(EditAnywhere, Category = "UI|Buff")
	TSubclassOf<UBuffListWidget> BuffListWidgetClass;
	UPROPERTY()
	UBuffListWidget* BuffListUI = nullptr;

	// 单人闯关模式右上角：当前关卡 + 最高历史记录（仅 TankStageGameMode 显示）
	UPROPERTY(EditAnywhere, Category = "UI|Campaign")
	TSubclassOf<UPassWidget> PassWidgetClass;
	UPROPERTY()
	UPassWidget* PassWidget = nullptr;

	// KDA 显示 UI（多人战斗使用）
	UPROPERTY(EditAnywhere, Category = "UI|KDA")
	TSubclassOf<UKDAWidget> KDAWidgetClass;
	UPROPERTY()
	UKDAWidget* KDAWidget = nullptr;

	// 重写 SetPawn，用于在获得新角色时刷新 UI
	virtual void SetPawn(APawn* InPawn) override;

	// 初始化 HUD UI（当 Pawn 在 BeginPlay 之后才 Possess 时调用）
	void InitializeHUD();

	// 刷新 KDA（无参，自动从 PlayerState 读取数据）
	UFUNCTION(BlueprintCallable, Category = "UI|KDA")
	void UpdateKDA();


	// --- 手柄震动反馈 ---
	// 发射时的微抖动强度 (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HapticFeedback")
	float FireVibrationIntensity = 0.3f;
	// 受伤时的中度抖动强度 (0-1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HapticFeedback")
	float DamageVibrationIntensity = 0.6f;
	// 发射震动持续时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HapticFeedback")
	float FireVibrationDuration = 0.1f;
	// 受伤震动持续时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HapticFeedback")
	float DamageVibrationDuration = 0.3f;

	// 触发手柄震动（发射时调用）
	UFUNCTION(BlueprintCallable, Category = "HapticFeedback")
	void TriggerFireVibration();

	UFUNCTION(Client, Unreliable)
	void ClientTriggerFireFeedback();

	// 触发手柄震动（受伤时调用）
	UFUNCTION(BlueprintCallable, Category = "HapticFeedback")
	void TriggerDamageVibration();

	// 停止手柄震动
	UFUNCTION(BlueprintCallable, Category = "HapticFeedback")
	void StopVibration();

	// 暂停菜单UI
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UPauseMenuWidget> PauseMenuClass;
	// 保存实例化的 暂停菜单UI 的 Widget 指针
	UPROPERTY(VisibleAnywhere)
	UPauseMenuWidget* PauseMenuInstance;
	// 【新增】需要在编辑器里把 IMC_Default 拖进去
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* InputMappingContext;
	// 【新增】需要在编辑器里把 IA_Pause 拖进去
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PauseAction;
	// 【新增】需要在编辑器里把 IA_Spectator 拖进去
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SpectatorAction;

	// ================= MOBA 模式 UI =================

	// 死亡复活界面（倒计时）
	UPROPERTY(EditAnywhere, Category = "UI|MOBA")
	TSubclassOf<class UDeathScreenWidget> DeathScreenClass;
	UPROPERTY()
	UDeathScreenWidget* DeathScreenInstance;

	// 永久淘汰界面
	UPROPERTY(EditAnywhere, Category = "UI|MOBA")
	TSubclassOf<class UEliminatedScreenWidget> EliminatedScreenClass;
	UPROPERTY()
	UEliminatedScreenWidget* EliminatedScreenInstance;

	// 显示死亡界面
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void ShowDeathScreen(float RespawnTime);

	UFUNCTION(Client, Reliable)
	void ClientShowDeathScreen(float RespawnTime);

	// 隐藏死亡界面
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void HideDeathScreen();

	UFUNCTION(Client, Reliable)
	void ClientHideDeathScreen();

	// 更新死亡界面倒计时
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void UpdateDeathScreenCountdown(float TimeRemaining);

	UFUNCTION(Client, Unreliable)
	void ClientUpdateDeathScreenCountdown(float TimeRemaining);

	// 显示淘汰界面
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void ShowEliminatedScreen();

	// 隐藏淘汰界面
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void HideEliminatedScreen();

	// 切换旁观者视角（手柄 X 键触发）
	UFUNCTION(BlueprintCallable, Category = "UI|MOBA")
	void EnterSpectatorMode();

	// ================= 回城系统 (Return To Spawn) =================
	// 回城输入动作（需要在蓝图编辑器中拖入 IA_ReturnToSpawn）
	UPROPERTY(EditAnywhere, Category = "Input|ReturnToSpawn")
	UInputAction* ReturnToSpawnAction;

	// 回城进度 Widget 类（需要在蓝图编辑器中拖入 WBP_ReturnProgress）
	UPROPERTY(EditAnywhere, Category = "ReturnToSpawn|UI")
	TSubclassOf<UReturnToSpawnWidget> ReturnProgressWidgetClass;

	// 回城进度 Widget 实例
	UPROPERTY()
	UReturnToSpawnWidget* ReturnProgressWidgetInstance = nullptr;

	// 按住时间阈值（秒）
	UPROPERTY(EditAnywhere, Category = "ReturnToSpawn")
	float ReturnToSpawnHoldTime = 7.0f;

	// 当前按住时间
	float CurrentHoldTime = 0.0f;

	// 是否正在按住回城键
	bool bIsHoldingReturnToSpawn = false;

	// 回城功能：开始按住
	void OnReturnToSpawnStarted(const FInputActionValue& Value);

	// 回城功能：结束按住/取消
	void OnReturnToSpawnCompleted(const FInputActionValue& Value);

	// 执行回城传送
	void ExecuteReturnToSpawn();

	// 获取关联的 Tank Pawn
	ATank* GetControlledTank() const;

	// 获取 Tank 是否存活
	bool IsTankAlive() const;

	// 检查 Tank 是否有出生点
	bool HasTankSpawnLocation() const;

protected:
	virtual void SetupInputComponent() override;
	// 触发暂停函数
	void TogglePauseMenu();

	void TickDeathScreenCountdown();

	// PlayerController 的 Tick（用于回城进度更新）
	virtual void PlayerTick(float DeltaTime) override;

private:
	FTimerHandle DeathScreenCountdownTimerHandle;
	float DeathScreenCountdownStartTime = 0.0f;
	float DeathScreenCountdownDuration = 0.0f;
};
