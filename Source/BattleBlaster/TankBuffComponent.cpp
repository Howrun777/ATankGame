/**
 * @file TankBuffComponent.cpp
 * @brief 坦克Buff组件实现 - 管理坦克的所有Buff效果
 * 
 * 核心逻辑:
 * 1. TickComponent: 每帧倒计时所有持续性Buff,检测过期并移除
 * 2. AddBuff: 根据Buff类型分别处理一次性Buff和持续性Buff
 * 3. 移速Buff特殊处理: 最后10秒线性衰减至基础速度
 */

#include "TankBuffComponent.h"
#include "BattleBlasterCollisionChannels.h"
#include "Tank.h"
#include "HealthComponent.h"
#include "TankPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Components/AudioComponent.h"

/**
 * @brief 构造函数 - 初始化组件
 * 
 * 启用Tick以便进行Buff倒计时
 */
UTankBuffComponent::UTankBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

/**
 * @brief 每帧更新 - 处理Buff倒计时
 * @param DeltaTime 帧间隔时间(秒)
 * 
 * 处理逻辑:
 * 1. 遍历所有持续性Buff,减去经过的时间
 * 2. 对移速Buff进行特殊处理: 最后10秒线性衰减
 * 3. 移除已过期的Buff并清除其效果
 */
void UTankBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 待移除的Buff列表
	TArray<EBuffType> BuffsToRemove;

	// 遍历所有持续性Buff
	for (auto& Pair : ActiveBuffs)
	{
		// 倒计时
		Pair.Value.RemainingTime -= DeltaTime;

		// ================== 移速Buff特殊处理:最后10秒线性衰减 ==================
		if (Pair.Key == EBuffType::Speed)
		{
			ATank* Tank = Cast<ATank>(GetOwner());
			if (Tank)
			{
				if (Pair.Value.RemainingTime <= 10.0f)
				{
					// 最后10秒:计算衰减比例 (例如剩5秒时,Ratio=0.5)
					float Ratio = Pair.Value.RemainingTime / 10.0f;

					// 线性插值计算当前速度
					// 剩10秒时:速度 = 基础速度 * 2 (200%)
					// 剩0秒时:速度 = 基础速度 * 1 (100%)
					Tank->Speed = Tank->BaseSpeed + (Tank->BaseSpeed * Ratio);
				}
				else
				{
					// 剩余时间大于10秒,保持200%速度
					Tank->Speed = Tank->BaseSpeed * 2.0f;
				}
			}
		}

		// 检查Buff是否已过期
		if (Pair.Value.RemainingTime <= 0.0f)
		{
			BuffsToRemove.Add(Pair.Key);
		}
	}

	// 移除已过期的Buff及其效果
	for (EBuffType Type : BuffsToRemove)
	{
		RemoveSustainedBuffEffect(Type);
		ActiveBuffs.Remove(Type);
	}

	// ==================== 窒息状态逻辑 ====================
	if (bIsInSuffocation)
	{
		// 更新窒息剩余时间
		SuffocationRemainingTime -= DeltaTime;

		// 检测是否已逃离物体
		if (!IsStuckInGeometry())
		{
			// 玩家已离开物体，真正移除 GhostMode
			OnEscapedFromGeometry();
		}
		else if (SuffocationRemainingTime <= 0.0f)
		{
			// 窒息时间耗尽，强制移除 GhostMode
			UE_LOG(LogTemp, Warning, TEXT("窒息时间耗尽，强制移除GhostMode"));
			if (OwnerTank)
			{
				OwnerTank->bIsGhostMode = false;
			}
			bIsInSuffocation = false;
			SuffocationRemainingTime = 0.0f;
			GhostModeBuffType = EBuffType::None;
			HideSuffocationUI();
		}
		else
		{
			// 扣除血量
			SuffocationDamageTimer += DeltaTime;
			if (SuffocationDamageTimer >= SuffocationDamageInterval)
			{
				SuffocationDamageTimer = 0.0f;

				// 扣除血量
				UHealthComponent* HealthComp = OwnerTank ? OwnerTank->FindComponentByClass<UHealthComponent>() : nullptr;
				if (HealthComp)
				{
					// 使用 ApplyDamage 来扣血
					UGameplayStatics::ApplyDamage(OwnerTank, SuffocationDamagePerTick, nullptr, OwnerTank, nullptr);
					UE_LOG(LogTemp, Warning, TEXT("窒息扣血: %.1f, 剩余时间: %.1f"), SuffocationDamagePerTick, SuffocationRemainingTime);
				}
			}
		}
	}
}

/**
 * @brief 添加Buff
 * @param BuffType Buff类型
 * @param Duration 持续时间(秒)
 * @param Icon Buff图标(用于UI显示)
 * 
 * 处理逻辑:
 * 1. 一次性Buff(Heal/Shield): 立即生效并应用效果,不加入持续列表
 * 2. 持续性Buff: 如果已存在则叠加时间,否则新建并应用效果
 */
void UTankBuffComponent::AddBuff(EBuffType BuffType, float Duration, UTexture2D* Icon)
{
	// 1. 一次性Buff: 立即生效,不需要加入持续列表
	if (BuffType == EBuffType::Heal || BuffType == EBuffType::Shield)
	{
		ApplyInstantBuff(BuffType);
		return;
	}

	// 2. 持续性Buff处理
	if (ActiveBuffs.Contains(BuffType))
	{
		// Buff已存在:叠加时间(延长持续时间)
		ActiveBuffs[BuffType].RemainingTime += Duration;
	}
	else
	{
		// Buff不存在:新建并应用效果
		FActiveBuffUIInfo NewBuff;
		NewBuff.Type = BuffType;
		NewBuff.RemainingTime = Duration;
		NewBuff.Icon = Icon;

		ActiveBuffs.Add(BuffType, NewBuff);
		ApplySustainedBuffEffect(BuffType);
	}
}

/**
 * @brief 应用一次性Buff效果
 * @param BuffType Buff类型(仅处理Heal和Shield)
 * 
 * 效果说明:
 * - Heal: 恢复生命值至100%
 * - Shield: 添加最大生命值50%的临时护盾
 */
void UTankBuffComponent::ApplyInstantBuff(EBuffType BuffType)
{
	OwnerTank = Cast<ATank>(GetOwner());
	if (!OwnerTank) return;

	// 生命恢复Buff
	if (BuffType == EBuffType::Heal)
	{
		UHealthComponent* HealthComp = OwnerTank->FindComponentByClass<UHealthComponent>();
		if (HealthComp)
		{
			HealthComp->ResetHealth();
			UE_LOG(LogTemp, Warning, TEXT("Buff生效: 生命值已恢复至100%%!"));
		}
	}
	// 护盾Buff
	else if (BuffType == EBuffType::Shield)
	{
		UHealthComponent* HealthComp = OwnerTank->FindComponentByClass<UHealthComponent>();
		if (HealthComp)
		{
			// 护盾量 = 最大生命值的50%
			float ShieldAmount = HealthComp->MaxHealth * 0.5f;
			HealthComp->AddShield(ShieldAmount);
		}
	}
}

/**
 * @brief 应用持续性Buff的开始效果
 * @param BuffType Buff类型
 * 
 * 根据不同类型设置坦克的对应标记或属性:
 * - Ammo: 开启无限子弹标记,UI显示9999
 * - Speed: 设置速度为基础速度的200%
 * - Pierce: 开启子弹穿透标记
 * - Ghost: 开启穿墙模式标记
 * - Damage: 开启伤害提升标记
 * - DoubleShot: 开启双发弹道标记
 */
void UTankBuffComponent::ApplySustainedBuffEffect(EBuffType BuffType)
{
	OwnerTank = Cast<ATank>(GetOwner());
	if (!OwnerTank) return;

	switch (BuffType)
	{
	case EBuffType::Ammo: // 无限子弹
		// 开启无限子弹标记
		OwnerTank->bHasInfiniteAmmo = true;
		// UI显示假数字9999让玩家感觉更强
		if (OwnerTank->TankPC) 
		{
			OwnerTank->TankPC->SetHUDAmmo(9999, OwnerTank->MaxAmmo);
		}
		break;

	case EBuffType::Speed: // 移速提升
		// 设置速度为基础速度的200%
		OwnerTank->Speed = OwnerTank->BaseSpeed * 2.0f;
		break;

	case EBuffType::Pierce: // 子弹穿透
		// 开启子弹穿透标记
		OwnerTank->bHasBulletPierce = true;
		break;

	case EBuffType::Ghost: // 穿墙模式
		// 开启穿墙模式标记
		OwnerTank->bIsGhostMode = true;
		// 2. 获取坦克的物理碰撞组件（通常是根组件 Capsule 或 Box）
		if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OwnerTank->GetRootComponent()))
		{
			// 【核心修复】：不要用 ECR_Ignore！改用 ECR_Overlap！
			// 这样既能穿透物理实体（不会被挡住），又能触发场景里陷阱和防御塔的警戒网！
			RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
			RootComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
			RootComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);

			UE_LOG(LogTemp, Warning, TEXT("物理穿墙已开启：已将碰撞改为 Overlap，保持雷达检测有效"));
		}
		break;

	case EBuffType::Damage: // 伤害提升
		// 开启伤害提升标记
		OwnerTank->bHasDamageBoost = true;
		break;

	case EBuffType::DoubleShot: // 双发弹道
		// 开启双发弹道标记
		OwnerTank->bHasDoubleShot = true;
		break;
	}
}

/**
 * @brief 移除持续性Buff的结束效果
 * @param BuffType Buff类型
 * 
 * 根据不同类型清除坦克的对应标记或恢复属性:
 * - Ammo: 关闭无限子弹标记,恢复真实子弹数显示
 * - Speed: 不需要手动恢复(在Tick中已通过线性衰减处理)
 * - Pierce: 关闭子弹穿透标记
 * - Ghost: 关闭穿墙模式标记
 * - Damage: 关闭伤害提升标记
 * - DoubleShot: 关闭双发弹道标记
 */
void UTankBuffComponent::RemoveSustainedBuffEffect(EBuffType BuffType)
{
	OwnerTank = Cast<ATank>(GetOwner());
	if (!OwnerTank) return;

	switch (BuffType)
	{
	case EBuffType::Ammo: // 无限子弹结束
		// 关闭无限子弹标记
		OwnerTank->bHasInfiniteAmmo = false;
		// 恢复显示真实子弹数(击杀奖励加的子弹也会正确显示)
		if (OwnerTank->TankPC) 
		{
			OwnerTank->TankPC->SetHUDAmmo(OwnerTank->CurrentAmmo, OwnerTank->MaxAmmo);
		}
		break;

	case EBuffType::Speed: // 移速Buff结束
		// 不需要手动恢复速度 - TickComponent中当RemainingTime<=0时,
		// Ratio=0,速度会自动恢复到基础速度(BaseSpeed * 1)
		// 这样可以避免瞬间重置打断玩家输入
		break;

	case EBuffType::Damage: // 伤害提升结束
		// 关闭伤害提升标记
		OwnerTank->bHasDamageBoost = false;
		break;

	case EBuffType::Pierce: // 子弹穿透结束
		// 关闭子弹穿透标记
		OwnerTank->bHasBulletPierce = false;
		break;

	case EBuffType::Ghost: // 穿墙模式结束
	{
		// 检查是否卡在物体内部
		if (IsStuckInGeometry())
		{
			// 进入窒息状态
			bIsInSuffocation = true;
			SuffocationRemainingTime = MaxSuffocationTime;
			SuffocationDamageTimer = 0.0f;
			GhostModeBuffType = EBuffType::Ghost;

			// 【极其重要】：卡在墙里时，必须保持穿墙状态为 true，千万别在这里恢复碰撞！
			OwnerTank->bIsGhostMode = true;

			// 显示窒息UI和音效
			ShowSuffocationUI();
			UE_LOG(LogTemp, Warning, TEXT("GhostMode结束但角色卡在墙内，进入窒息状态！"));
		}
		else
		{
			// 【核心修复】：正常在空地上结束穿墙，也必须恢复物理碰撞！
			OwnerTank->bIsGhostMode = false;

			// ==========================================
			// 恢复实心碰撞！
			if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OwnerTank->GetRootComponent()))
			{
				RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
				RootComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
				RootComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);
			}
			// ==========================================

			UE_LOG(LogTemp, Warning, TEXT("GhostMode正常结束，物理实体已恢复！"));
		}
		break;
	}

	case EBuffType::DoubleShot: // 双发弹道结束
		// 关闭双发弹道标记
		OwnerTank->bHasDoubleShot = false;
		break;
	}
}

/**
 * @brief 获取当前激活的Buff列表(供UI使用)
 * @return 包含所有持续性Buff信息的数组
 * 
 * 用于HUD或UI显示当前有哪些激活的Buff及其剩余时间
 */
TArray<FActiveBuffUIInfo> UTankBuffComponent::GetActiveBuffsForUI()
{
	TArray<FActiveBuffUIInfo> Result;
	ActiveBuffs.GenerateValueArray(Result);
	return Result;
}

/**
 * @brief 清除所有Buff - 用于关卡切换或游戏重置
 * 
 * 处理逻辑:
 * 1. 遍历所有Buff,调用RemoveSustainedBuffEffect清除效果
 * 2. 清空ActiveBuffs列表
 * 
 * 使用场景:
 * - 玩家过关时继承Buff到下一关(先清除再重新添加)
 * - 游戏重置时清除所有状态
 */
void UTankBuffComponent::ClearAllBuffs()
{
	// 移除所有Buff的效果
	for (auto& Pair : ActiveBuffs)
	{
		RemoveSustainedBuffEffect(Pair.Key);
	}

	// 清空列表
	ActiveBuffs.Empty();
}

/**
 * @brief 获取所有持续性Buff的信息(用于复活时恢复)
 * @return 包含所有持续性Buff类型和剩余时间的数组
 */
TArray<FActiveBuffUIInfo> UTankBuffComponent::GetAllActiveBuffs()
{
	TArray<FActiveBuffUIInfo> Result;
	ActiveBuffs.GenerateValueArray(Result);
	return Result;
}

/**
 * @brief 从保存的Buff信息恢复Buff(用于复活)
 * @param SavedBuffs 保存的Buff信息数组
 */
void UTankBuffComponent::RestoreBuffs(const TArray<FActiveBuffUIInfo>& SavedBuffs)
{
	for (const FActiveBuffUIInfo& SavedBuff : SavedBuffs)
	{
		if (SavedBuff.Type == EBuffType::None || SavedBuff.Type == EBuffType::Heal || SavedBuff.Type == EBuffType::Shield)
		{
			continue;
		}

		if (ActiveBuffs.Contains(SavedBuff.Type))
		{
			ActiveBuffs[SavedBuff.Type].RemainingTime += SavedBuff.RemainingTime;
		}
		else
		{
			ActiveBuffs.Add(SavedBuff.Type, SavedBuff);
			ApplySustainedBuffEffect(SavedBuff.Type);
		}
	}
}

/**
 * @brief 检测角色当前是否在其他物体内
 * @return true 如果角色与其他物体发生重叠
 */
bool UTankBuffComponent::IsStuckInGeometry()
{
	if (!OwnerTank) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	// 获取坦克的当前位置
	FVector Location = OwnerTank->GetActorLocation();

	// ==========================================
	// 【核心修复】：把检测中心往上抬，避开地面！
	// 假设坦克中心点离地面很近，我们强行把检测点抬高 60 厘米
	Location.Z += 60.0f;

	// 缩小检测半径，只检测车体核心（避免稍微擦到墙皮就误判）
	float CheckRadius = 45.0f;
	// ==========================================

	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(CheckRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerTank); // 忽略坦克自己
	QueryParams.bTraceComplex = false;

	// 强制进行空间重叠检测
	bool bIsOverlappingWall = World->OverlapAnyTestByChannel(
		Location,
		OwnerTank->GetActorQuat(),
		ECC_WorldStatic,
		CollisionShape,
		QueryParams
	);

	return bIsOverlappingWall;
}
/**
 * @brief 窒息状态下从墙体中逃离后的回调
 * 当玩家离开物体后，真正移除 GhostMode 效果
 */
void UTankBuffComponent::OnEscapedFromGeometry()
{
	if (!bIsInSuffocation) return;

	// 1. 真正移除 GhostMode 效果
	if (OwnerTank)
	{
		// 关闭穿墙逻辑标记
		OwnerTank->bIsGhostMode = false;

		// ==========================================
		// 【核心修复】：玩家出墙了，把物理碰撞通道改回阻挡！
		if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(OwnerTank->GetRootComponent()))
		{
			RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			RootComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
			RootComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);
		}
		// ==========================================
	}

	// 2. 清除窒息状态
	bIsInSuffocation = false;
	SuffocationRemainingTime = 0.0f;
	SuffocationDamageTimer = 0.0f;
	GhostModeBuffType = EBuffType::None;

	// 3. 隐藏窒息UI并停止音效
	HideSuffocationUI();

	UE_LOG(LogTemp, Warning, TEXT("玩家已离开墙体，GhostMode移除，物理实体已恢复！"));
}


/**
 * @brief 显示窒息UI并播放音效
 */
void UTankBuffComponent::ShowSuffocationUI()
{
	if (!OwnerTank || !GetWorld()) return;

	// 1. 播放窒息音效，并把“播放器”存到 ActiveSuffocationSoundComp 里
	// 只有存起来了，等玩家出去的时候我们才能叫它 Stop()
	if (SuffocationSound && !ActiveSuffocationSoundComp)
	{
		// SpawnSoundAttached 会返回一个正在播放的音频组件 (UAudioComponent)
		ActiveSuffocationSoundComp = UGameplayStatics::SpawnSoundAttached(
			SuffocationSound,               // 用你的 USoundBase 变量
			OwnerTank->GetRootComponent(),  // 绑定在坦克身上
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true                            // 坦克被摧毁时自动停止
		);
	}

	// 2. 创建并显示窒息UI
	if (SuffocationWidgetClass && !CurrentSuffocationWidget)
	{
		APlayerController* PC = Cast<APlayerController>(OwnerTank->GetController());
		if (PC)
		{
			CurrentSuffocationWidget = CreateWidget<UUserWidget>(PC, SuffocationWidgetClass); // 用你的 UUserWidget 变量
			if (CurrentSuffocationWidget)
			{
				CurrentSuffocationWidget->AddToViewport(100); // 高优先级显示
			}
		}
	}
}

void UTankBuffComponent::HideSuffocationUI()
{
	// 1. 隐藏并销毁 UI
	if (CurrentSuffocationWidget)
	{
		CurrentSuffocationWidget->RemoveFromParent();
		CurrentSuffocationWidget = nullptr;
	}

	// 2. 停止播放窒息音效！
	if (ActiveSuffocationSoundComp)
	{
		ActiveSuffocationSoundComp->Stop(); // 掐断声音！
		ActiveSuffocationSoundComp = nullptr; // 清空指针，下次进墙还能重新触发
	}
}
