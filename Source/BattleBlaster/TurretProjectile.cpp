#include "TurretProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HealthComponent.h"
#include "TankMOBAPlayerState.h"
#include "Tank.h"
#include "TeamBattleGameMode.h"
#include "TankMOBAGameMode.h"
#include "BattleBlasterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

ATurretProjectile::ATurretProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	MaxLifeTime = 5.0f;
	LifeTime = 0.0f;
	Speed = 500.0f;
	Damage = 25.0f;
	CampIndex = -1;
	TargetActor = nullptr;
	

	// 创建碰撞组件
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	RootComponent = ProjectileMesh;
	ProjectileMesh->SetNotifyRigidBodyCollision(true);

	// 创建移动组件
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;

	// 碰撞回调
	ProjectileMesh->OnComponentHit.AddDynamic(this, &ATurretProjectile::OnHit);

	// 创建拖尾特效组件
	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(RootComponent); // 附加到根组件或 ProjectileMesh 上
	TrailComponent->bAutoActivate = true; // 默认自动激活
}

void ATurretProjectile::BeginPlay()
{
	Super::BeginPlay();
	LifeTime = 0.0f;
	// 💡新增：让炮弹的物理碰撞彻底忽略炮塔，防止炮弹把炮塔挤开
	if (GetOwner() && ProjectileMesh)
	{
		ProjectileMesh->IgnoreActorWhenMoving(GetOwner(), true);
	}
	// 播放发射音效
	if (LaunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), LaunchSound, GetActorLocation());
	}

	// 播放发射特效
	if (LaunchEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), LaunchEffect, GetActorLocation());
	}
}

void ATurretProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 增加存活时间
	LifeTime += DeltaTime;

	// 超过最大存活时间，销毁
	if (LifeTime >= MaxLifeTime)
	{
		Destroy();
		return;
	}

	// 追踪目标
	if (IsValid(TargetActor))
	{
		// 检查目标是否是Pawn且存活
		APawn* TargetPawn = Cast<APawn>(TargetActor);
		if (TargetPawn && !TargetPawn->IsActorBeingDestroyed())
		{
			// 更新朝向
			FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			ProjectileMovement->Velocity = Direction * Speed;
		}
		else
		{
			// 目标已死亡，继续直线飞行
		}
	}
}

void ATurretProjectile::InitializeProjectile(AActor* InTargetActor, float InDamage, float InSpeed, int32 InCampIndex)
{
	this->TargetActor = InTargetActor;
	this->Damage = InDamage;
	this->Speed = InSpeed;
	this->CampIndex = InCampIndex;

	// 设置移动速度
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = Speed;
		ProjectileMovement->MaxSpeed = Speed;
	}

	// 初始朝向目标
	if (IsValid(InTargetActor))
	{
		FVector Direction = (InTargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		ProjectileMovement->Velocity = Direction * Speed;
		SetActorRotation(Direction.Rotation());
	}
}

void ATurretProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 如果打中的是无效物体、或者是炮弹自己、或者是发射炮弹的炮塔，直接无视！
	if (!IsValid(OtherActor) || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	// 💡 优化建议：如果是拖尾，最好是脱离父项并停止发射，而不是直接 Destroy，
	// 这样已经发射出来的粒子可以自然消散，看起来更平滑。
	if (TrailComponent)
	{
		// 从炮弹上解绑，保留在世界中
		TrailComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		// 停止发射新粒子，等老粒子生命周期结束自动销毁组件
		TrailComponent->Deactivate();
	}


	// 检查是否击中Tank
	APawn* TankPawn = Cast<APawn>(OtherActor);
	if (TankPawn)
	{
		// 获取当前游戏模式
		AGameModeBase* CurrentGM = GetWorld()->GetAuthGameMode();

		// 获取目标坦克
		ATank* VictimTank = Cast<ATank>(TankPawn);

		bool bCanDamage = false;

		// MOBA 模式：每个玩家独立阵营，同 PlayerIndex = 同阵营
		if (Cast<ATankMOBAGameMode>(CurrentGM))
		{
			if (VictimTank && VictimTank->GetPlayerIndex() >= 0)
			{
				// MOBA 模式：防御塔的 CampIndex 应该等于目标的 PlayerIndex
				// 不同 PlayerIndex 可以互相伤害
				bCanDamage = (CampIndex != VictimTank->GetPlayerIndex());
			}
		}
		// TeamBattle 模式：同阵营（0/2=红色，1/3=蓝色）不能互相伤害
		else if (Cast<ATeamBattleGameMode>(CurrentGM))
		{
			if (VictimTank && VictimTank->GetPlayerIndex() >= 0)
			{
				bool bVictimIsRed = (VictimTank->GetPlayerIndex() == 0 || VictimTank->GetPlayerIndex() == 2);
				bool bAttackerIsRed = (CampIndex == 0 || CampIndex == 2);
				bCanDamage = (bAttackerIsRed != bVictimIsRed);
			}
		}
		// 其他模式：都可以互相伤害
		else
		{
			bCanDamage = true;
		}

		if (bCanDamage)
		{
			// 获取炮塔作为 InstigatorController 的拥有者
			AActor* TurretOwner = GetOwner();
			AController* InstigatorController = nullptr;
			if (TurretOwner)
			{
				InstigatorController = TurretOwner->GetInstigatorController();
			}

			// 造成伤害，传递正确的 InstigatorController
			UGameplayStatics::ApplyDamage(
				OtherActor, Damage,
				InstigatorController,
				TurretOwner,  // ← 从 this 改成 TurretOwner（就是 GetOwner()）
				UDamageType::StaticClass()
			);

			UE_LOG(LogTemp, Display, TEXT("TurretProjectile: Applied damage %f to %s, InstigatorController: %s"),
				Damage, *OtherActor->GetName(), *GetNameSafe(InstigatorController));

			// 播放击中特效
			if (HitEffect)
			{
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, GetActorLocation());
			}

			// 播放击中音效
			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
			}
		}
	}

	// 销毁投射物
	Destroy();
}
