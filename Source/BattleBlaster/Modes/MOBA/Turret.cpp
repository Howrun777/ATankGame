#include "Modes/MOBA/Turret.h"
#include "Shared/Pawns/NPC/Tower.h"
#include "Modes/MOBA/TankMOBAGameState.h"
#include "Modes/MOBA/TankMOBAPlayerState.h"
#include "Modes/MOBA/TurretProjectile.h"
#include "Shared/Pawns/Tank.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

ATurret::ATurret()
{
	PrimaryActorTick.bCanEverTick = true;

	CampIndex = 0;
	bIsCoreTurret = false;
	AttackInterval = 1.0f;
	VisionRadius = 1000.0f;
	ProjectileSpeed = 500.0f;
	AttackDamage = 25.0f;
	bIsAttacking = false;
	CurrentTarget = nullptr;
	HealPercent = 0.5f;

	// 默认在编辑器中显示攻击范围
	bShowVisionRangeInEditor = true;
	bCanToggleVisionRangeInGame = true;

	// 创建发射点组件（可拖拽）
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(RootComponent);
	MuzzlePoint->SetRelativeLocation(FVector(0, 0, 100));

	// 偏移位置
	MuzzleOffset = FVector(0, 0, 100);

	// 创建视野范围组件
	VisionRangeComp = CreateDefaultSubobject<USphereComponent>(TEXT("VisionRange"));
	VisionRangeComp->SetupAttachment(RootComponent);
	VisionRangeComp->SetSphereRadius(VisionRadius);
	VisionRangeComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 创建死亡特效生成点（可拖拽，默认在根组件上方 100 单位）
	DeathEffectSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DeathEffectSpawnPoint"));
	DeathEffectSpawnPoint->SetupAttachment(RootComponent);
	DeathEffectSpawnPoint->SetRelativeLocation(FVector(0, 0, 100));
}

void ATurret::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 在编辑器中更新发射点位置
	if (MuzzlePoint)
	{
		MuzzlePoint->SetRelativeLocation(MuzzleOffset);
	}

	// 更新攻击范围显示
	if (VisionRangeComp)
	{
		VisionRangeComp->SetSphereRadius(VisionRadius);

#if WITH_EDITOR
		// 编辑器中总是显示
		VisionRangeComp->SetVisibility(bShowVisionRangeInEditor);
#else
		// 游戏开始时根据设置显示
		VisionRangeComp->SetVisibility(false);
#endif
	}
}

void ATurret::BeginPlay()
{
	Super::BeginPlay();

	// 获取游戏状态
	MOBAGameState = GetWorld()->GetGameState<ATankMOBAGameState>();

	// 在游戏状态中注册此防御塔
	if (MOBAGameState)
	{
		MOBAGameState->RegisterTurret(this);
		
		// 手动增加防御塔计数
		if (bIsCoreTurret)
		{
			int32 CurrentCount = MOBAGameState->CoreTurretCountByCamp.FindRef(CampIndex);
			MOBAGameState->CoreTurretCountByCamp.Add(CampIndex, CurrentCount + 1);
		}
		else
		{
			int32 CurrentCount = MOBAGameState->OuterTurretCountByCamp.FindRef(CampIndex);
			MOBAGameState->OuterTurretCountByCamp.Add(CampIndex, CurrentCount + 1);
		}
	}

	// 更新发射点位置
	UpdateMuzzleLocation();

	// 初始化视野范围显示
	InitializeVisionRangeVisualization();

	// 默认开启攻击
	StartAttacking();
}

void ATurret::UpdateMuzzleLocation()
{
	if (MuzzlePoint)
	{
		MuzzlePoint->SetRelativeLocation(MuzzleOffset);
	}
}

void ATurret::InitializeVisionRangeVisualization()
{
	if (VisionRangeComp)
	{
		VisionRangeComp->SetSphereRadius(VisionRadius);

		// 游戏开始时默认隐藏，由玩家手动切换
#if WITH_EDITOR
		VisionRangeComp->SetVisibility(bShowVisionRangeInEditor);
#else
		VisionRangeComp->SetVisibility(false);
#endif
	}
}

void ATurret::ToggleVisionRange()
{
	if (!bCanToggleVisionRangeInGame || !VisionRangeComp)
	{
		return;
	}

	bool bCurrentVisibility = VisionRangeComp->IsVisible();
	VisionRangeComp->SetVisibility(!bCurrentVisibility);
}

void ATurret::SetVisionRangeVisible(bool bVisible)
{
	if (VisionRangeComp)
	{
		VisionRangeComp->SetVisibility(bVisible);
	}
}

void ATurret::StartAttacking()
{
	if (!bIsAttacking)
	{
		bIsAttacking = true;
		
		// 立即检测一次
		DetectAndAttack();

		// 启动定时器
		if (AttackInterval > 0.0f)
		{
			GetWorld()->GetTimerManager().SetTimer(
				AttackTimerHandle,
				this,
				&ATurret::DetectAndAttack,
				AttackInterval,
				true
			);
		}
	}
}

void ATurret::StopAttacking()
{
	bIsAttacking = false;
	GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
}

void ATurret::DetectAndAttack()
{
	if (!IsValid(this))
	{
		return;
	}

	// 💡 核心修复：如果当前锁定的目标是Tank，且它已经死亡，立刻清空目标！停止鞭尸！
	if (ATank* CurrentTank = Cast<ATank>(CurrentTarget))
	{
		if (!CurrentTank->IsAlive)
		{
			CurrentTarget = nullptr;
		}
	}

	// 如果没有目标或目标已物理销毁，搜索新目标
	APawn* TargetPawn = Cast<APawn>(CurrentTarget);
	if (!IsValid(CurrentTarget) || !TargetPawn || TargetPawn->IsActorBeingDestroyed())
	{
		CurrentTarget = nullptr;

		TArray<AActor*> FoundActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), FoundActors);

		float ClosestDistance = MAX_flt;
		AActor* BestTarget = nullptr;

		for (AActor* Actor : FoundActors)
		{
			// 跳过所有 Tower，让 Turret 只攻击 Tank
			if (Cast<ATower>(Actor))
			{
				continue;
			}

			// 排除自己、队友，以及死亡的玩家
			if (ShouldAttackTarget(Actor))
			{
				float Distance = FVector::Dist(GetActorLocation(), Actor->GetActorLocation());

				if (Distance <= VisionRadius && Distance < ClosestDistance)
				{
					ClosestDistance = Distance;
					BestTarget = Actor;
				}
			}
		}

		CurrentTarget = BestTarget;
	}

	// 如果有有效目标，发射投射物
	if (IsValid(CurrentTarget))
	{
		float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());
		if (Distance <= VisionRadius)
		{
			FireProjectile();
		}
		else
		{
			CurrentTarget = nullptr;
		}
	}
}

bool ATurret::ShouldAttackTarget(AActor* Target) const
{
	if (!IsValid(Target) || Target == this)
	{
		return false;
	}

	// Turret 只攻击 Tank，不攻击 Tower
	if (Cast<ATower>(Target))
	{
		return false;
	}

	// 绝对不要把死人列入攻击白名单！
	if (ATank* TargetTank = Cast<ATank>(Target))
	{
		if (!TargetTank->IsAlive)
		{
			return false;
		}
	}

	// 获取目标所属的阵营
	int32 TargetCampIndex = -1;

	// 直接使用 Tank 的 PlayerIndex（比分屏模式下的 PlayerState 更可靠）
	ATank* TargetTank = Cast<ATank>(Target);
	if (TargetTank)
	{
		TargetCampIndex = TargetTank->GetPlayerIndex();
	}
	// 如果不是 Tank，通过 PlayerState 获取（用于 AI 等其他 Pawn）
	else
	{
		APawn* TargetPawn = Cast<APawn>(Target);
		if (TargetPawn && TargetPawn->GetPlayerState())
		{
			class ATankMOBAPlayerState* MOBAState = Cast<ATankMOBAPlayerState>(TargetPawn->GetPlayerState());
			if (MOBAState)
			{
				TargetCampIndex = MOBAState->GetCampIndex();
			}
		}
	}

	// 目标不是自己阵营的，才攻击
	return TargetCampIndex != CampIndex;
}

void ATurret::FireProjectile()
{
	if (!IsValid(ProjectileClass) || !IsValid(CurrentTarget))
	{
		return;
	}

	// 获取发射位置
	FVector SpawnLocation = GetMuzzleLocation();
	FRotator SpawnRotation = (CurrentTarget->GetActorLocation() - SpawnLocation).Rotation();

	// 💡新增：配置生成参数
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this; // 关键：将炮塔设置为炮弹的所有者！
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // 即使有碰撞也强制生成

	// 生成投射物，传入 SpawnParams
	ATurretProjectile* Projectile = GetWorld()->SpawnActor<ATurretProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (Projectile)
	{
		// 初始化投射物
		Projectile->InitializeProjectile(CurrentTarget, AttackDamage, ProjectileSpeed, CampIndex);
	}
}

FVector ATurret::GetMuzzleLocation() const
{
	// 优先使用 MuzzlePoint 组件
	if (MuzzlePoint)
	{
		return MuzzlePoint->GetComponentLocation();
	}

	// 备选：使用 Socket
	UStaticMeshComponent* MeshComp = FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp && MeshComp->DoesSocketExist(ProjectileMuzzleSocket))
	{
		return MeshComp->GetSocketLocation(ProjectileMuzzleSocket);
	}
	
	// 默认：使用 Actor 位置 + 偏移
	return GetActorLocation() + MuzzleOffset;
}

void ATurret::SetTarget(AActor* NewTarget)
{
	CurrentTarget = NewTarget;
}

bool ATurret::CanAttackTarget(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}
	
	// 检查是否是Pawn并存活
	APawn* TargetPawn = Cast<APawn>(Target);
	if (!TargetPawn || TargetPawn->IsActorBeingDestroyed())
	{
		return false;
	}
	
	return ShouldAttackTarget(Target);
}

bool ATurret::IsDamageImmune() const
{
	// 只有主防御塔才有伤害免疫
	if (!bIsCoreTurret)
	{
		return false;
	}

	// 检查游戏状态
	if (!MOBAGameState)
	{
		return false;
	}

	// 检查是否有外防御塔存活
	int32 OuterCount = MOBAGameState->GetAliveOuterTurretCountByCamp(CampIndex);
	
	// 如果有外防御塔存活，主防御塔免疫伤害
	return OuterCount > 0;
}

void ATurret::UpdateDamageImmunity()
{
	// 可在蓝图中添加视觉反馈（如防护罩特效）
	// 这里的实现由蓝图完成
}

void ATurret::HandleDestruction()
{
	// 【第一步】立即停止开火，防止死后还在发射
	StopAttacking();

	// 【第二步】根据炮塔类型选择废墟网格体并替换
	UStaticMesh* RuinsMesh = bIsCoreTurret ? CoreTurretRuinsMesh : TurretRuinsMesh;

	if (RuinsMesh && PropMesh)
	{
		PropMesh->SetStaticMesh(RuinsMesh);

		// 废墟网格体设为静态，不接收移动/旋转，减少物理开销
		PropMesh->SetMobility(EComponentMobility::Static);
	}

	// 【第三步】播放销毁音效（一次性播放，不挂载到 Actor 上，避免生命周期管理开销）
	if (DestructionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			DestructionSound,
			GetActorLocation(),
			FRotator::ZeroRotator,
			1.0f,   // VolumeMultiplier
			1.0f,   // PitchMultiplier
			0.0f,   // StartTime
			nullptr,// AttenuationSettings（可后续在蓝图配置）
			nullptr // ConcurrencySettings
		);
	}

	// 【第三步半】播放死亡 Niagara 特效（以 SpawnPoint 为中心）
	if (DeathEffect)
	{
		FVector SpawnLocation = DeathEffectSpawnPoint
			? DeathEffectSpawnPoint->GetComponentLocation()
			: GetActorLocation();
		FRotator SpawnRotation = DeathEffectSpawnPoint
			? DeathEffectSpawnPoint->GetComponentRotation()
			: FRotator::ZeroRotator;

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DeathEffect,
			SpawnLocation,
			SpawnRotation,
			FVector(1.0f),  // Scale
			true,           // bAutoDestroy
			true,           // bAutoActivate
			ENCPoolMethod::AutoRelease,
			true            // bPreCullCheck
		);
	}

	// 【第四步】通知游戏状态塔已被摧毁
	if (MOBAGameState)
	{
		MOBAGameState->OnTurretDestroyed(this);
	}

	// 【第五步】调用父类的破坏处理（关闭碰撞、隐藏血量条）
	Super::HandleDestruction();

	// 【第六步】废墟保持碰撞效果：玩家和子弹仍会被废墟阻挡
	// 必须在 Super::HandleDestruction() 之后执行，否则会被父类覆盖
	if (RuinsMesh && PropMesh)
	{
		PropMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PropMesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
}

float ATurret::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 检查是否是主防御塔且处于免疫状态
	if (bIsCoreTurret && IsDamageImmune())
	{
		// 免疫伤害，但仍然调用父类以触发一些通用逻辑（如更新UI）
		UE_LOG(LogTemp, Display, TEXT("CoreTurret is immune to damage!"));
		return 0.0f;
	}

	// 检查攻击者是否是友方Tank
	if (EventInstigator && EventInstigator->GetPawn())
	{
		AActor* AttackerPawn = EventInstigator->GetPawn();
		ATank* AttackerTank = Cast<ATank>(AttackerPawn);
		
		if (AttackerTank)
		{
			int32 AttackerCamp = AttackerTank->GetPlayerIndex();
			
			// 如果是同阵营，治疗防御塔
			if (AttackerCamp == CampIndex)
			{
				// 计算治疗量
				float HealAmount = DamageAmount * HealPercent;
				
				// 治疗
				if (HealthComp)
				{
					HealthComp->Heal(HealAmount);
					UpdateHealthBar();
					UE_LOG(LogTemp, Display, TEXT("Turret healed by %f (from %f damage)"), HealAmount, DamageAmount);
				}
				
				// 不造成伤害
				return 0.0f;
			}
		}
	}

	// 正常造成伤害
	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}
