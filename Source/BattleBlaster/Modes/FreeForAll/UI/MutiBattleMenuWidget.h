#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"




// 注意这里必须是这个名字
#include "MutiBattleMenuWidget.generated.h"

// 提前声明组件，加快编译速度
class UButton;
class UTextBlock;
class UImage; // 新增：提前声明图片组件
class UTexture2D;


// 用来描述一个可供选择的 Tank（图标 + 蓝图类）
USTRUCT(BlueprintType)
struct FTankOption
{
	GENERATED_BODY()

	// 显示在 UI 上的 Tank 头像
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	UTexture2D* Icon = nullptr;

	// 这个图标对应的 Tank 蓝图类（必须继承自 Pawn/Tank）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TankSelect")
	TSubclassOf<APawn> TankClass = nullptr;
};


UCLASS()
class BATTLEBLASTER_API UMutiBattleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 【新增】上一个页面：多人模式选择菜单类
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Navigation")
	TSubclassOf<UUserWidget> PreviousMenuClass;
	// 在蓝图里指定你的地图选择 UI 类
	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<class USelectMapWidget> MapSelectWidgetClass;
	// 指定多人死斗对应的 GameMode 是哪一个！
	UPROPERTY(EditAnywhere, Category = "UI GameMode")
	TSubclassOf<AGameModeBase> MultiplayerGameModeClass;
	//注意这里应该传入多人死斗的游戏模式类蓝图:BP_BattleBlasterGameMode

	UFUNCTION()
	void OnTankSelectAxisInput(int32 SlotId, float AxisValue);

	// 【新增】通知 GameInstance 注册 SlotId → DeviceId 映射
	UFUNCTION(BlueprintCallable, Category = "TankSelect|DeviceMapping")
	void NotifyPlayerInputDevice(int32 SlotId, FInputDeviceId DeviceId);
	
	// 在现有成员变量区域添加
	// UI 认为的可用玩家数量（由 CurrentPlayerCount 决定，而不是实际手柄数）
	UPROPERTY()
	int32 UIConfiguredPlayerCount = 1;

	// ================= 新增：背景图片管理 =================
	// 在蓝图里配置的背景图片数组 (索引 0=2人背景, 1=3人背景, 2=4人背景)
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|Backgrounds")
	TArray<UTexture2D*> BackgroundImages;

protected:
	// ================= 设备图标（手柄 / AI） =================
	// 在蓝图里配置：索引0=玩家手柄图，索引1=AI图
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|DeviceIcons")
	TArray<TObjectPtr<UTexture2D>> DeviceIconTextures;

	// 绑定底层的背景图片控件 (必须在 UMG 里放一个 Image 并命名为 BGImage)
	UPROPERTY(meta = (BindWidget))
	class UImage* BGImage;
	// 【新增】：用于覆盖在上面做渐隐特效的图片
	UPROPERTY(meta = (BindWidget))
	class UImage* BGImage_Overlay;
	// 渐变动画的辅助变量
	bool bIsFading = false;
	float CurrentFadeAlpha = 0.0f;
	// 控制渐变的速度 (越大消失得越快，3.0 表示大约 0.33 秒切完)
	UPROPERTY(EditDefaultsOnly, Category = "UI Setup|Backgrounds")
	float FadeSpeed = 7.0f;

	void UpdateBackgroundImage();

	virtual void NativeConstruct() override;
	// 启用 UI 的 Tick（每帧执行）功能
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ================== Tank 选择相关控件 ==================
	// 注意：名字要与 UMG 蓝图里左下角四张图片一一对应
	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_1;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_2;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_3;

	UPROPERTY(meta = (BindWidget))
	UImage* TankImage_4;

	// ================= 新增：绑定高亮边框 =================
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_1;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_2;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_3;
	UPROPERTY(meta = (BindWidget))
	class UBorder* HoverFrame_4;

	// 专门处理玩家1（鼠标）的定向滚轮选择
	void HandleMouseWheelTargeting(float DeltaTime);

	// 鼠标滚轮独立的冷却计时器（防止一滚滚过头）
	float MouseWheelCooldownTimer = 0.0f;

	// ================== Tank 选项配置（在蓝图填） ==================
	// 所有可选 Tank 的图标 + 蓝图类列表
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TankSelect")
	TArray<FTankOption> TankOptions;
	// ================== 每个玩家当前的选择下标 ==================
	// PlayerTankIndices[0] -> 玩家1当前选中的 TankOptions 下标
	// PlayerTankIndices[1] -> 玩家2 ...
	UPROPERTY()
	TArray<int32> PlayerTankIndices;
	// 实际可用玩家数量（由手柄数量和逻辑决定）
	UPROPERTY()
	int32 ActivePlayerCount = 1;

	// ========== 输入参数：死区 & 冷却 ==========
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float AxisDeadZone = 0.3f;
	UPROPERTY(EditAnywhere, Category = "TankSelect|Input")
	float SwitchCooldown = 0.25f; // 每次切换间隔
	// 记录每个玩家距离上次切换已经过去的时间
	UPROPERTY()
	TArray<float> PlayerSwitchTimers;

	// ================== 内部辅助方法 ==================
	// 初始化 PlayerTankIndices / PlayerSwitchTimers 大小
	void InitPlayerTankState(int32 InPlayerCount);
	// 按玩家序号刷新对应 Tank 图片
	void UpdateTankImageForPlayer(int32 SlotId);
	// 刷新所有玩家 Tank 图片
	void UpdateAllTankImages();
	// 每帧处理所有玩家的选择输入
	void HandleTankSelectionInput(float DeltaTime);
	// 单个玩家的输入处理：playerIndex 对应 PlayerController 索引
	void HandleSinglePlayerInput(int32 SlotId, float DeltaTime);

	// 用于冷却：记录每个玩家上一次切换的时间戳（秒）
	UPROPERTY()
	TArray<float> LastSwitchTimestamp;
	// 确保本地玩家数量足够（多手柄时必须 CreatePlayer）
	void EnsureLocalPlayers(int32 WantedPlayers);

	// ==================== 绑定蓝图控件 ====================
	// 这里的名字必须和 UMG 蓝图里的名字完全一致！

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Confirm;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_Back;			// 对应截图里的 But_Back
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_PlayerMinus;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_PlayerPlus;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_PlayerCount;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScoreMinus;
	UPROPERTY(meta = (BindWidget))
	UButton* Btn_ScorePlus;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Text_ScoreCount;
	// ================= 绑定手柄图片 =================
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_1;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_2;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_3;
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Gamepad_4;

	// ==================== 内部数据逻辑 ====================

	// 当前设置面板显示的人数和分数（默认值）
	int32 CurrentPlayerCount = 2;
	int32 CurrentScore = 7;

	// 更新文本显示
	void UpdateDisplay();

	// ==================== 按钮点击事件 ====================
	// 绑定给按钮的事件必须加上 UFUNCTION() 宏

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnBackClicked();

	UFUNCTION()
	void OnPlayerMinusClicked();

	UFUNCTION()
	void OnPlayerPlusClicked();

	UFUNCTION()
	void OnScoreMinusClicked();

	UFUNCTION()
	void OnScorePlusClicked();

	// ================= 手柄检测逻辑 =================
	// 获取当前物理连接的设备数量
	static constexpr float DeviceCountRefreshInterval = 0.5f;
	float DeviceCountRefreshTimer = 0.0f;
	int32 CachedConnectedDeviceCount = 1;

	void RefreshConnectedDeviceCount();
	int32 GetConnectedDeviceCount();
	int32 GetDesiredLocalPlayerCount() const;

	// 根据数量刷新图片的显示和隐藏
	void UpdateDeviceIcons(int32 DeviceCount);

};

