#include "Shared/Pawns/Tank.h"
#include "Core/BattleBlasterCollisionChannels.h"
#include "Blueprint/UserWidget.h"


#include "Shared/Combat/HealthComponent.h"
#include "Camera/CameraComponent.h"
#include "Shared/Combat/Projectile.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"//为了调用这个函数GetWorldDeltaSeconds
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/State/TankPlayerState.h"
#include "Modes/MOBA/TankMOBAGameMode.h"
#include "Modes/FreeForAll/BattleBlasterGameMode.h"

// ========== PlayerState 数据访问方法实现 ==========

int32 ATank::GetPlayerIndex() const
{
	// 优先返回 Tank 自身存储的 PlayerIndex（Spawn 时就设置好了）
	if (PlayerIndex >= 0)
	{
		return PlayerIndex;
	}

	// 如果 Tank 的 PlayerIndex 还没设置，尝试从 PlayerState 获取
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->PlayerIndex;
	}
	return -1;
}

bool ATank::GetIsAlive() const
{
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->IsAlive;
	}
	return IsAlive;
}

void ATank::SetIsAlive(bool bAlive)
{
	IsAlive = bAlive;
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		PS->SetAlive(bAlive);
	}
}

FVector ATank::GetHomeSpawnLocation() const
{
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->GetHomeSpawnLocation();
	}
	return GetActorLocation();
}

FRotator ATank::GetHomeSpawnRotation() const
{
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->GetHomeSpawnRotation();
	}
	return GetActorRotation();
}

bool ATank::HasSpawnLocation() const
{
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->HasSpawnLocation();
	}
	return false;
}

int32 ATank::GetAmmo() const
{
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		return PS->GetAmmo();
	}
	return CurrentAmmo;
}

void ATank::SetAmmo(int32 NewAmmo)
{
	CurrentAmmo = NewAmmo;
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		PS->UpdateAmmo(NewAmmo);
	}
}

ATank::ATank()
{
	//创建弹簧臂组件并且赋值给Tank类成员
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	//SpringArmComp->SetupAttachment(CapsuleComp);//把这个组件附加到Tank的炮塔
	SpringArmComp->SetupAttachment(TurretMesh);//把这个组件附加到Tank的炮塔
	//创建相机组件,并且赋值到Tank类成员
	SpringArmComp->TargetArmLength = 400.0f; // 确保拉开距离
	//SpringArmComp->bUsePawnControlRotation = true; // 跟随控制器旋转

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	// 炮管开镜相机：挂到发射点（炮管口），与炮管朝向一致；默认禁用，开镜时再启用
	ScopeCameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("ScopeCameraComp"));
	ScopeCameraComp->SetupAttachment(ProjectileSpawnPoint);
	ScopeCameraComp->SetActive(false);
	ScopeCameraComp->bUsePawnControlRotation = false;

	// TEXT 里面的名字是在虚幻编辑器细节面板里显示的内部名字，叫 "BuffComponent" 即可
	BuffComp = CreateDefaultSubobject<UTankBuffComponent>(TEXT("BuffComponent"));

	// ================= NavMesh 支持：创建移动组件 =================
	PawnMovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("PawnMovementComponent"));
	PawnMovementComponent->MaxSpeed = Speed;
	PawnMovementComponent->bConstrainToPlane = true;
	PawnMovementComponent->SetPlaneConstraintNormal(FVector(0.0f, 0.0f, 1.0f)); // 限制在 XY 平面移动
	PawnMovementComponent->UpdatedComponent = RootComponent;
}

void ATank::BeginPlay()
{
	Super::BeginPlay();
	if (CapsuleComp)
	{
		CapsuleComp->SetCollisionObjectType(ECC_Pawn);
		CapsuleComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);
	}
	BaseSpeed = Speed;

	// === 优先从 PlayerState 读取已保存的出生点 ===
	// 注意：多数 GameMode 先 Spawn 再 Possess，此处 GetPlayerState 可能仍为空；
	// 真正可靠的写入在 PossessedBy（见上文）。
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		if (!PS->HasSpawnLocation())
		{
			// PlayerState 中没有出生点，记录当前 Actor 的位置作为出生点
			PS->RecordSpawnLocation(GetActorLocation(), GetActorRotation());
		}
		// 从 PlayerState 读取弹药（如果 PlayerState 中有有效值）
		if (PS->CurrentAmmo > 0)
		{
			CurrentAmmo = PS->CurrentAmmo;
		}
	}

	// === 读取 GameMode 的复活百分比 ===
	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GMBase = World->GetAuthGameMode())
		{
			float HealthPercent = 1.0f;
			float AmmoPercent = 1.0f;

			if (ABattleBlasterGameMode* BBGM = Cast<ABattleBlasterGameMode>(GMBase))
			{
				HealthPercent = BBGM->RespawnHealthPercent;
				AmmoPercent = BBGM->RespawnAmmoPercent;
			}
			else if (ATankMOBAGameMode* MOBAGM = Cast<ATankMOBAGameMode>(GMBase))
			{
				HealthPercent = MOBAGM->RespawnHealthPercent;
				AmmoPercent = MOBAGM->RespawnAmmoPercent;
			}

			if (HealthComp) // 【修复】：使用父类的 HealthComp
			{
				HealthComp->CurrentHealth = HealthComp->MaxHealth * HealthPercent;
			}
			// 只有在 PlayerState 中没有有效弹药时才用默认值
			if (CurrentAmmo <= 0)
			{
				CurrentAmmo = MaxAmmo * AmmoPercent;
			}
		}
	}

	// === 【关键修复】：只绑定一次新版事件，且使用 HealthComp ===
	if (HealthComp)
	{
		HealthComp->OnHealthChanged.AddDynamic(this, &ATank::HandleHealthChanged);
		HealthComp->OnDeath.AddDynamic(this, &ATank::HandleDeath);
		//UE_LOG(LogTemp, Warning, TEXT("Tank::BeginPlay - Health events bound successfully"));
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("Tank::BeginPlay - HealthComp is NULL!"));
	}
}


void ATank::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	

	// 确保在 Tank 被销毁时移除瞄准镜 UI，防止空指针访问
	if (ScopeWidgetInstance && ScopeWidgetInstance->IsInViewport())
	{
		ScopeWidgetInstance->RemoveFromParent();
		ScopeWidgetInstance = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ATank::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 【回城/PlayerState】BeginPlay 往往在 Possess 之前执行，此时 Pawn 尚未关联 PlayerState，
	// GetPlayerState<ATankPlayerState>() 多为 nullptr，出生点从未写入 → HasSpawnLocation() 一直为 false。
	// 在挂上 Controller 后补记一次；若已记录（同局复活等）则绝不覆盖，保留首次基地坐标。
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		if (!PS->HasSpawnLocation())
		{
			PS->RecordSpawnLocation(GetActorLocation(), GetActorRotation());
		}
	}

	// 1. 尝试获取本地玩家控制器
	TankPC = Cast<ATankPlayerController>(NewController);

	if (TankPC && TankPC->IsLocalController())
	{
		TankPC->SetHUDAmmo(CurrentAmmo, MaxAmmo);
		// 2. 获取该玩家对应的 增强输入子系统
		ULocalPlayer* LocalPlayer = TankPC->GetLocalPlayer();
		if (LocalPlayer)
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				/*
				* 由于OpenLevel()关卡切换时,会导致Tank玩家的输入映射上下文会被销毁, 
				  所以需要重新绑定,不然就无法控制角色
				*/
				// 3. 关键：先清除旧的上下文（防止切换关卡残留），再添加新的
				Subsystem->ClearAllMappings();
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
				}
			}
		}

		// 4. 设置输入模式
		FInputModeGameOnly InputMode;
		TankPC->SetInputMode(InputMode);
		TankPC->bShowMouseCursor = false;

		TankPC->SetHUDAmmo(CurrentAmmo, MaxAmmo);
		// 【新增】：刷新初始生命值 UI！
		if (HealthComp)
		{
			float InitHealthPct = HealthComp->MaxHealth > 0.0f ? (HealthComp->CurrentHealth / HealthComp->MaxHealth) : 0.0f;
			float InitShieldPct = HealthComp->MaxShield > 0.0f ? (HealthComp->CurrentShield / HealthComp->MaxShield) : 0.0f;

			TankPC->UpdateHealthHUD(InitHealthPct, InitShieldPct);
		}
	}
}

void ATank::SetPlayerIndex(int32 NewPlayerIndex)
{
	// 同时设置 Tank 自身的 PlayerIndex（用于阵营判断的快速访问）
	PlayerIndex = NewPlayerIndex;

	// 同步到 PlayerState（真正的权威数据源）
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		PS->PlayerIndex = NewPlayerIndex;
	}
}

//击杀奖励
void ATank::HandleKillReward()
{
	// 1. 增加子弹 (默认10发)
	// 【注意】：后台真实的子弹依然要加，因为等无限火力结束了玩家还是要用的！
	CurrentAmmo += AmmoReward;

	// 限制子弹不超过上限
	if (CurrentAmmo > MaxAmmo)
	{
		CurrentAmmo = MaxAmmo;
	}

	// 同步弹药到 PlayerState
	SetAmmo(CurrentAmmo);

	// 更新 UI 上的子弹数
	if (TankPC)
	{
		// ================== 【核心修复2：拦截机制】 ==================
		// 如果当前是有无限子弹Buff的，就不要去刷真实子弹数了！
		if (bHasInfiniteAmmo)
		{
			// 如果你强迫症想保持9999闪一下，可以写这一句：
			TankPC->SetHUDAmmo(9999, MaxAmmo);
		}
		else
		{
			// 只有正常状态下，才把真实的子弹数刷到屏幕上
			TankPC->SetHUDAmmo(CurrentAmmo, MaxAmmo);
		}
		// ===============================================================
	}

	// 2. 恢复生命值 (25点)
	if (HealthComp)
	{
		HealthComp->Heal(HealthReward);
	}

	//UE_LOG(LogTemp, Warning, TEXT("Enemy Killed! Reward: +10 Ammo, +25 Health"));
}
// 绑定输入函数
void ATank::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 1. 获取玩家控制器（在这个函数里，Controller 肯定已经赋值了）
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		// 2. 获取增强输入子系统
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// 3. 添加映射上下文 (Mapping Context)
			// 只要加上这一步，WASD 就会生效
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 绑定具体的按键功能
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 记得检查指针是否为空，防止蓝图没赋值导致崩溃
		if (MoveAction) EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATank::MoveInput);
		if (TurnAction) EIC->BindAction(TurnAction, ETriggerEvent::Triggered, this, &ATank::TurnInput);
		if (FireAction) EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ATank::Fire);
		if (TurretTurnAction) EIC->BindAction(TurretTurnAction, ETriggerEvent::Triggered, this, &ATank::TurretTurnInput);
		// 开镜：键鼠点击切换
		if (IA_Aim_Toggle) EIC->BindAction(IA_Aim_Toggle, ETriggerEvent::Started, this, &ATank::OnAimToggle);
		// 开镜：手柄按住
		if (IA_Aim_Hold)
		{
			EIC->BindAction(IA_Aim_Hold, ETriggerEvent::Started, this, &ATank::OnAimHoldStarted);
			EIC->BindAction(IA_Aim_Hold, ETriggerEvent::Completed, this, &ATank::OnAimHoldCompleted);
			EIC->BindAction(IA_Aim_Hold, ETriggerEvent::Canceled, this, &ATank::OnAimHoldCompleted);
		}
	}
}
//移动函数（玩家输入，经 EnhancedInput 触发）
void ATank::MoveInput(const FInputActionValue& Value)
{
	// 通过 PlayerController 检查回城状态
	if (ATankPlayerController* PC = Cast<ATankPlayerController>(GetController()))
	{
		if (PC->bIsHoldingReturnToSpawn)
		{
			return;
		}
	}

	float InputValue = Value.Get<float>();

	const float DeltaSeconds = (GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f);
	if (FMath::IsNearlyZero(DeltaSeconds) || FMath::IsNearlyZero(InputValue))
	{
		return;
	}

	// 【修复1】：确保坦克的前进方向严格保持在水平面 (XY平面)
	FVector MoveDir = GetActorForwardVector();
	MoveDir.Z = 0.0f; // 强制抹除Z轴朝向
	MoveDir.Normalize();

	const FVector DesiredWorldDelta = MoveDir * (Speed * InputValue * DeltaSeconds);

	// 普通状态：bSweep = true，移动会被墙体阻挡
	// 穿墙 Buff 状态：bSweep = false，移动时不再进行碰撞扫掠，可以直接穿过墙体
	const bool bSweep = !bIsGhostMode;

	FHitResult Hit;
	AddActorWorldOffset(DesiredWorldDelta, bSweep, &Hit);

	// 贴墙滑动：如果被阻挡，把剩余位移投影到墙面切线方向，避免“一擦就卡死”
	if (bSweep && bEnableWallSlide && Hit.IsValidBlockingHit())
	{
		const float RemainingTime = FMath::Clamp(1.0f - Hit.Time, 0.0f, 1.0f);
		const FVector RemainderDelta = DesiredWorldDelta * RemainingTime;

		// 计算滑动向量
		FVector SlideDelta = FVector::VectorPlaneProject(RemainderDelta, Hit.Normal) * WallSlideSpeedScale;

		// 【核心修复2】：无论碰撞法线如何，强制抹除滑动时的Z轴位移，严格限制在二维平面滑动
		SlideDelta.Z = 0.0f;

		if (!SlideDelta.IsNearlyZero(0.1f))
		{
			AddActorWorldOffset(SlideDelta, true);
		}
	}
}

// 移动函数（AI 控制器直接调用）
void ATank::MoveAI(const FVector2D& MoveInput)
{
	// 这里只用 Y 分量作为前进/后退，保持和玩家一致
	float InputValue = MoveInput.Y;

	const float DeltaSeconds = (GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f);
	if (FMath::IsNearlyZero(DeltaSeconds) || FMath::IsNearlyZero(InputValue))
	{
		return;
	}

	// 【修复1】：确保前进方向无 Z 轴分量
	FVector MoveDir = GetActorForwardVector();
	MoveDir.Z = 0.0f;
	MoveDir.Normalize();

	const FVector DesiredWorldDelta = MoveDir * (Speed * InputValue * DeltaSeconds);

	const bool bSweep = !bIsGhostMode;

	FHitResult Hit;
	AddActorWorldOffset(DesiredWorldDelta, bSweep, &Hit);

	if (bSweep && bEnableWallSlide && Hit.IsValidBlockingHit())
	{
		const float RemainingTime = FMath::Clamp(1.0f - Hit.Time, 0.0f, 1.0f);
		const FVector RemainderDelta = DesiredWorldDelta * RemainingTime;

		FVector SlideDelta = FVector::VectorPlaneProject(RemainderDelta, Hit.Normal) * WallSlideSpeedScale;

		// 【核心修复2】：强制抹除滑动时的 Z 轴位移
		SlideDelta.Z = 0.0f;

		if (!SlideDelta.IsNearlyZero(0.1f))
		{
			AddActorWorldOffset(SlideDelta, true);
		}
	}
}
//转弯函数
void ATank::TurnInput(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();

	FRotator DeltaAngle = FRotator(0.0f, 0.0f, 0.0f);

	DeltaAngle.Yaw = TurnRate * InputValue * GetWorld()->GetDeltaSeconds();

	AddActorLocalRotation(DeltaAngle);

}
//炮塔转动函数
void ATank::TurretTurnInput(const FInputActionValue& Value)
{
	float TargetSpeed = Value.Get<float>() * TurnRate;

	// 开镜时降低炮塔转速到 30%，方便精细瞄准
	if (bIsAiming)
	{
		TargetSpeed *= 0.3f;
	}

	// 对“速度”进行插值：从 当前速度 过渡到 目标速度
	CurrentTurnSpeed = FMath::FInterpTo(
		CurrentTurnSpeed,
		TargetSpeed,
		GetWorld()->GetDeltaSeconds(),
		1000.0f // 这个值越小，炮塔启动和停止的惯性越大
	);

	// 如果速度很小，就别算了
	if (FMath::IsNearlyZero(CurrentTurnSpeed)) return;

	// 根据当前的平滑速度，计算这一帧转多少
	float RotationAmount = CurrentTurnSpeed * GetWorld()->GetDeltaSeconds();

	// 累加旋转
	TurretMesh->AddRelativeRotation(FRotator(0.0f, RotationAmount, 0.0f));
}

//死亡函数
void ATank::HandleDestruction() {
	Super::HandleDestruction();
	//UE_LOG(LogTemp, Warning, TEXT("Tank::HandleDestruction called for %s!"), *GetName());

	// 隐藏坦克的可见网格体和组件
	// 注意：这不同于Destroy()，对象仍然存在，只是不可见
	// 适用于需要保留对象但隐藏其视觉效果的情况（如玩家重生前）
	SetActorHiddenInGame(true);

	// 禁用坦克的Tick函数调用
	// Tick是每帧执行的更新函数，禁用后可以节省性能
	// 这对于隐藏或暂时不活动的对象是常见的优化手段
	SetActorTickEnabled(false);

	// 彻底关闭物理碰撞 
	// 这样子弹会穿过去，其他坦克也能直接开过去
	SetActorEnableCollision(false);

	// 禁用玩家的输入控制
	// 确保玩家在坦克"死亡"期间无法控制已隐藏的坦克
	SetPlayerEnabled(false);
	//设置玩家状态为阵亡
	IsAlive = false;
}

void ATank::SetPlayerEnabled(bool Enabled)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (Enabled)
		{
			// 确保游戏过程中鼠标点击不会先“抢焦点/捕获”导致需要点两下
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);

			// 恢复输入：重新添加 Mapping Context
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
				{
					// 添加默认映射上下文(用于Tank的基本输入)
					if (DefaultMappingContext)
					{
						Subsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
					}

					// 添加TankPlayerController中设置的InputMappingContext(包含暂停键等)
					// 这确保复活后仍然可以呼出暂停菜单
					if (ATankPlayerController* PlayerTankPC = Cast<ATankPlayerController>(PlayerController))
					{
						if (PlayerTankPC->InputMappingContext)
						{
							Subsystem->AddMappingContext(PlayerTankPC->InputMappingContext, 2);
						}
					}
				}
			}
			PlayerController->bShowMouseCursor = false;
		}
		else
		{
			// 禁用输入：清除 Mapping Context
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
				{
					Subsystem->ClearAllMappings();
				}
			}
			PlayerController->bShowMouseCursor = true;
			// 退出开镜并移除瞄准镜 UI
			bAimToggleOn = false;
			bAimHoldPressed = false;
			UpdateAimView();
		}
	}
}
void ATank::Fire()
{
	// 通过 PlayerController 检查回城状态
	if (ATankPlayerController* PC = Cast<ATankPlayerController>(GetController()))
	{
		if (PC->bIsHoldingReturnToSpawn)
		{
			return;
		}
	}

	// 判断发射几枚炮弹
	int32 ProjectileCount = bHasDoubleShot ? 2 : 1;

	// 检查是否有足够的子弹
	if (CurrentAmmo < ProjectileCount && !bHasInfiniteAmmo) {
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - Fire_LastTime < Fire_Interval) {
		return;
	}

	FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
	FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

	// 计算双发弹道的偏移方向（使用发射点组件的右侧向量）
	FVector RightVector = ProjectileSpawnPoint->GetRightVector();
	float ProjectileSpacing = 50.0f; // 两枚炮弹之间的距离

	// 在发射炮弹之前，先扣除子弹（只扣一次，避免双发时重复扣除）
	if (!bHasInfiniteAmmo)
	{
		CurrentAmmo -= ProjectileCount; // 双发扣除2发，单发扣除1发
		// 同步弹药到 PlayerState
		SetAmmo(CurrentAmmo);
	}

	// ================== 【终极拦截：防9999被顶替】 ==================
	if (TankPC)
	{
		// 如果正在爽玩无限火力，就强行让屏幕上一直显示 9999！
		if (bHasInfiniteAmmo)
		{
			TankPC->SetHUDAmmo(9999, MaxAmmo);
		}
		// 否则，老老实实显示扣完之后的真实子弹数
		else
		{
			TankPC->SetHUDAmmo(CurrentAmmo, MaxAmmo);
		}
	}
	// =================================================================

	for (int32 i = 0; i < ProjectileCount; ++i)
	{
		// 计算当前炮弹的生成位置
		// 无双发buff(i=0): 偏移为0，对齐炮口
		// 有双发buff: i=0向左偏移, i=1向右偏移，中点在原位置
		float OffsetAmount = 0.0f;
		if (bHasDoubleShot)
		{
			OffsetAmount = (i == 0) ? (-ProjectileSpacing / 2.0f) : (ProjectileSpacing / 2.0f);
		}
		FVector AdjustedSpawnLocation = SpawnLocation + (RightVector * OffsetAmount);

		// 1. 准备好生成位置和旋转的 Transform
		FTransform SpawnTransform(SpawnRotation, AdjustedSpawnLocation);
		// 2. 延迟生成：炮弹被捏出来了，但在内存里处于"时停"状态，不会执行 BeginPlay
		AProjectile* Projectile = GetWorld()->SpawnActorDeferred<AProjectile>(ProjectileClass, SpawnTransform);
		if (Projectile)
		{
			Projectile->SetOwner(this);
			if (i == 0)
			{
				Fire_LastTime = CurrentTime;
				// 发射时触发手柄微抖动
				if (TankPC)
				{
					TankPC->TriggerFireVibration();
				}
			}

			// 3. 趁着"时停"，疯狂修改它的属性！
			if (bHasDamageBoost)
			{
				Projectile->Damage *= 2.0f;
				Projectile->EnableBoostVisuals(); // 把音效、特效全换成强化版！
			}

			// 子弹穿透 Buff：启用穿透模式（这里设为无限穿透）
			if (bHasBulletPierce)
			{
				// 参数说明：
				// 1) true  => 使用无限穿透模式（MaxPenetrationCount = -1）
				// 2) -1    => 此参数仅在 bInfinitePierce=false 时才起作用
				Projectile->EnablePierceMode(true, -1);
			}

			// 4. 修改完毕，解除"时停"，正式让炮弹进入游戏世界！
			// 此时炮弹才会执行 BeginPlay()，并播放我们刚换上去的强化版 LaunchSound
			UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
		}
	}
}

UPawnMovementComponent* ATank::GetMovementComponent() const
{
	return PawnMovementComponent;
}

void ATank::MoveWithAI(float ForwardInput, float RightInput)
{
	// 使用 AddMovementInput，让 AAIController 的 MoveToActor 能够驱动 Tank
	// 方向是相对于 Tank 自身的

	const FVector ForwardDir = GetActorForwardVector();
	const FVector RightDir = GetActorRightVector();

	FVector WorldDirection = (ForwardDir * ForwardInput) + (RightDir * RightInput);
	WorldDirection.Normalize();

	AddMovementInput(WorldDirection, 1.0f, false);
}

void ATank::NotifyAttacked(AActor* Attacker)
{
	// 如果攻击者是Projectile，获取它的Owner（发射炮弹的Tower或Tank）
	AActor* ActualAttacker = Attacker;
	if (Attacker && Attacker->IsA<AProjectile>())
	{
		ActualAttacker = Attacker->GetOwner();
		//UE_LOG(LogTemp, Warning, TEXT("Projectile owner: %s"), ActualAttacker ? *ActualAttacker->GetName() : TEXT("None"));
	}

	// 获取 AI Controller 并通知它被攻击了
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AAIBotPlayerController* BotPC = Cast<AAIBotPlayerController>(AIController);
		if (BotPC && ActualAttacker)
		{
			BotPC->OnAttackedBy(ActualAttacker);
		}
	}
}
void ATank::HandleHealthChanged(UHealthComponent* InHealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// 1. 【更新本地 UI】：只有玩家自己（本地控制）才需要更新屏幕上的血条！
	if (IsLocallyControlled() && TankPC && InHealthComp)
	{
		// 计算百分比 (防除零崩溃)
		float HealthPct = InHealthComp->MaxHealth > 0.0f ? (InHealthComp->CurrentHealth / InHealthComp->MaxHealth) : 0.0f;
		float ShieldPct = InHealthComp->MaxShield > 0.0f ? (InHealthComp->CurrentShield / InHealthComp->MaxShield) : 0.0f;

		// 告诉 Controller 去刷新 UI
		TankPC->UpdateHealthHUD(HealthPct, ShieldPct);
	}

	// 2. 如果是受伤，处理震动和找凶手
	if (HealthDelta < 0.0f)
	{
		// 通知 AI Controller 被攻击了，使其能够反击 Tower 等非 Tank 凶手
		NotifyAttacked(DamageCauser);

		if (IsLocallyControlled() && TankPC)
		{
			TankPC->TriggerDamageVibration();
		}

		AActor* KillerActor = nullptr;
		if (InstigatedBy)
		{
			KillerActor = InstigatedBy->GetPawn();
		}
		else if (DamageCauser)
		{
			// DamageCauser 可能是 Tank（正常击杀）也可能是 Tower/建筑（建筑击杀）
			KillerActor = Cast<ATank>(DamageCauser);  // 只接受 Tank，不接受建筑
		}

		if (ATank* KillerTank = Cast<ATank>(KillerActor))
		{
			if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
			{
				PS->RecordAttacker(KillerTank);
			}
		}
	}
}

void ATank::HandleDeath(UHealthComponent* InHealthComp, AController* InstigatedBy, AActor* DamageCauser)
{
	// 提取凶手 Tank 指针（用于后续 GameMode 胜负判定）
	CachedKiller = InstigatedBy ? Cast<ATank>(InstigatedBy->GetPawn()) : Cast<ATank>(DamageCauser);

	// 表现层死亡（爆炸、隐藏自己等）
	HandleDestruction();

	// KDA 结算（击杀/助攻/死亡数，由 PlayerState 内部处理）
	if (ATankPlayerState* PS = GetPlayerState<ATankPlayerState>())
	{
		PS->ProcessDeath();
	}

}

ATank* ATank::ExecuteDeathAndReturnKiller()
{
	// 仅返回凶手 Tank（GameMode 用它做胜负判定，不再重新触发 KDA）
	// KDA 已在 HandleDeath 中通过 ProcessDeath() 处理完毕
	ATank* KillerTank = CachedKiller;
	CachedKiller = nullptr;
	return KillerTank;
}


// ================= 开镜：键鼠切换 / 手柄按住 =================
void ATank::OnAimToggle(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;
	bAimToggleOn = !bAimToggleOn;
	UpdateAimView();
}

void ATank::OnAimHoldStarted(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;
	bAimHoldPressed = true;
	UpdateAimView();
}

void ATank::OnAimHoldCompleted(const FInputActionValue& Value)
{
	if (!IsLocallyControlled()) return;
	bAimHoldPressed = false;
	UpdateAimView();
}

void ATank::UpdateAimView()
{
	const bool bShouldAim = bAimToggleOn || bAimHoldPressed;
	if (bIsAiming == bShouldAim) return;
	bIsAiming = bShouldAim;

	// 切换相机：开镜用炮管相机，关镜用默认弹簧臂相机
	if (ScopeCameraComp && CameraComp)
	{
		if (bIsAiming)
		{
			CameraComp->SetActive(false);
			ScopeCameraComp->SetActive(true);
		}
		else
		{
			ScopeCameraComp->SetActive(false);
			CameraComp->SetActive(true);
		}
	}

	// 瞄准镜 UMG：开镜显示（带边缘模糊的 Widget 在蓝图中配置），关镜移除
	// 使用 AddToPlayerScreen 替代 AddToViewport，确保分屏时各玩家看到自己的 UI
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (ScopeWidgetClass && PC && PC->IsLocalController())
	{
		if (bIsAiming)
		{
			if (!ScopeWidgetInstance)
			{
				ScopeWidgetInstance = CreateWidget<UUserWidget>(PC, ScopeWidgetClass);
			}
			if (ScopeWidgetInstance && !ScopeWidgetInstance->IsInViewport())
			{
				ScopeWidgetInstance->AddToPlayerScreen();
			}
		}
		else
		{
			if (ScopeWidgetInstance && ScopeWidgetInstance->IsInViewport())
			{
				ScopeWidgetInstance->RemoveFromParent();
			}
		}
	}
}

