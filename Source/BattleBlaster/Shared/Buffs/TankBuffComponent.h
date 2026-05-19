/**
 * @file TankBuffComponent.h
 * @brief 坦克Buff组件 - 管理坦克的所有Buff效果
 * 
 * 功能说明:
 * - 管理两种类型的Buff: 一次性Buff(立即生效)和持续性Buff(有持续时间)
 * - 一次性Buff: Heal(生命恢复), Shield(护盾)
 * - 持续性Buff: Ammo(无限子弹), Speed(移速提升), Pierce(子弹穿透), 
 *               Ghost(穿墙模式), Damage(伤害提升), DoubleShot(双发弹道)
 * - 持续性Buff支持时间叠加,即获取相同Buff时延长时间
 * - 移速Buff最后10秒会线性衰减至基础速度
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shared/Buffs/BuffTypes.h"
#include "TankBuffComponent.generated.h"

class USoundBase;
class UUserWidget;

/**
 * @struct FActiveBuffUIInfo
 * @brief 用于UI显示的Buff信息结构体
 * 
 * 包含Buff类型、图标指针和剩余时间
 * 用于在HUD上显示当前激活的Buff列表
 */
USTRUCT(BlueprintType)
struct FActiveBuffUIInfo
{
	GENERATED_BODY()

	/** Buff类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBuffType Type = EBuffType::None;

	/** Buff图标 - 用于UI显示 */
	UPROPERTY()
	UTexture2D* Icon = nullptr;

	/** 剩余持续时间(秒) */
	UPROPERTY()
	float RemainingTime = 0.0f;
};

/**
 * @class UTankBuffComponent
 * @brief 坦克Buff管理组件
 * 
 * 负责:
 * - 添加/移除Buff
 * - 维护Buff持续时间
 * - 应用/移除Buff效果
 * - 提供Buff信息给UI系统
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLEBLASTER_API UTankBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTankBuffComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * @brief 每帧更新 - 处理Buff倒计时
	 * @param DeltaTime 帧间隔时间(秒)
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * @brief 添加Buff
	 * @param BuffType Buff类型
	 * @param Duration 持续时间(秒),一次性Buff此参数无效
	 * @param Icon Buff图标,用于UI显示
	 * 
	 * 逻辑:
	 * - 一次性Buff(Heal/Shield): 立即生效并应用效果,不加入持续列表
	 * - 持续性Buff: 如果已存在则叠加时间,否则新建并应用效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void AddBuff(EBuffType BuffType, float Duration, UTexture2D* Icon);

	/**
	 * @brief 获取当前激活的Buff列表(供UI使用)
	 * @return 包含所有持续性Buff信息的数组
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	TArray<FActiveBuffUIInfo> GetActiveBuffsForUI();

	/**
	 * @brief 清除所有Buff - 用于关卡切换或游戏重置
	 * 
	 * 会:
	 * - 移除所有Buff的持续效果(重置坦克属性)
	 * - 清空Buff列表
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void ClearAllBuffs();

	/**
	 * @brief 获取所有持续性Buff的信息(用于复活时恢复)
	 * @return 包含所有持续性Buff类型和剩余时间的数组
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	TArray<FActiveBuffUIInfo> GetAllActiveBuffs();

	/**
	 * @brief 从保存的Buff信息恢复Buff(用于复活)
	 * @param SavedBuffs 保存的Buff信息数组
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void RestoreBuffs(const TArray<FActiveBuffUIInfo>& SavedBuffs);

	/**
	 * @brief 检测角色当前是否在其他物体内
	 * @return true 如果角色与其他物体发生重叠
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	bool IsStuckInGeometry();

	/**
	 * @brief 窒息状态下从墙体中逃离后的回调
	 * 当玩家离开物体后，真正移除 GhostMode 效果
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	void OnEscapedFromGeometry();

	/**
	 * @brief 获取窒息状态剩余时间
	 * @return 剩余窒息时间，0表示不在窒息状态
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	float GetSuffocationRemainingTime() const { return SuffocationRemainingTime; }

	/**
	 * @brief 获取当前是否处于窒息状态
	 */
	UFUNCTION(BlueprintCallable, Category = "Buff")
	bool IsInSuffocation() const { return bIsInSuffocation; }

	/** 窒息伤害间隔时间(秒) */
	UPROPERTY(EditAnywhere, Category = "Buff|Suffocation")
	float SuffocationDamageInterval = 1.0f;

	/** 每次窒息扣除的血量 */
	UPROPERTY(EditAnywhere, Category = "Buff|Suffocation")
	float SuffocationDamagePerTick = 10.0f;

	/** 窒息状态最大持续时间(秒)，超时则强制移除GhostMode */
	UPROPERTY(EditAnywhere, Category = "Buff|Suffocation")
	float MaxSuffocationTime = 999.0f;

	/** 窒息UI类 */
	UPROPERTY(EditAnywhere, Category = "Buff|Suffocation")
	TSubclassOf<UUserWidget> SuffocationWidgetClass;

	/** 窒息音效 */
	UPROPERTY(EditAnywhere, Category = "Buff|Suffocation")
	USoundBase* SuffocationSound;
	// 【新增这一行】：这是音效的“播放器”，用于随时停止播放
	UPROPERTY()
	class UAudioComponent* ActiveSuffocationSoundComp = nullptr;
private:
	/** 存储当前激活的持续性Buff,Key为Buff类型,Value为Buff信息 */
	UPROPERTY()
	TMap<EBuffType, FActiveBuffUIInfo> ActiveBuffs;

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedActiveBuffs)
	TArray<FActiveBuffUIInfo> ReplicatedActiveBuffs;

	UFUNCTION()
	void OnRep_ReplicatedActiveBuffs();

	void RefreshReplicatedActiveBuffs();

	/** 是否处于窒息状态(穿墙Buff结束后卡在墙内) */
	bool bIsInSuffocation = false;

	/** 窒息状态剩余时间 */
	float SuffocationRemainingTime = 0.0f;

	/** 窒息伤害计时器 */
	float SuffocationDamageTimer = 0.0f;

	/** 正在经历的真正GhostMode效果的Buff类型(用于离开物体后清除) */
	EBuffType GhostModeBuffType = EBuffType::None;

	/** 当前显示的窒息UI实例 */
	UUserWidget* CurrentSuffocationWidget = nullptr;

	/** 显示窒息UI并播放音效 */
	void ShowSuffocationUI();

	/** 隐藏窒息UI */
	void HideSuffocationUI();

	/**
	 * @brief 应用一次性Buff效果
	 * @param BuffType Buff类型(仅处理Heal和Shield)
	 * 
	 * - Heal: 恢复生命值至100%
	 * - Shield: 添加最大生命值50%的护盾
	 */
	void ApplyInstantBuff(EBuffType BuffType);

	/**
	 * @brief 应用持续性Buff的开始效果
	 * @param BuffType Buff类型
	 * 
	 * 根据不同类型设置坦克的对应标记或属性
	 */
	void ApplySustainedBuffEffect(EBuffType BuffType);

	/**
	 * @brief 移除持续性Buff的结束效果
	 * @param BuffType Buff类型
	 * 
	 * 根据不同类型清除坦克的对应标记或恢复属性
	 */
	void RemoveSustainedBuffEffect(EBuffType BuffType);

	/** 持有此组件的坦克指针(缓存以提高性能) */
	class ATank* OwnerTank = nullptr;
};
