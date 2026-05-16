
#include "Shared/Combat/Projectile.h"
#include "Core/BattleBlasterCollisionChannels.h"





#include "Shared/Pawns/Tank.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Modes/MOBA/TankMOBAPlayerState.h"
#include "Modes/TeamBattle/TeamBattleGameMode.h"
#include "Modes/MOBA/TankMOBAGameMode.h"
#include "Modes/FreeForAll/BattleBlasterGameMode.h"
#include "Shared/World/DestructibleProp.h"
/* @brief 炮弹Actor的构造函数
 * @note 核心作用：初始化炮弹的核心组件（网格体、移动组件、拖尾粒子组件），并设置基础运动参数
 *       构造函数在Actor创建时执行，仅初始化组件和默认参数，不处理运行时逻辑
 */

AProjectile::AProjectile()
{
    // 炮弹移动由 ProjectileMovementComp 负责，命中由 OnHit 处理，不需要 Actor Tick。
    PrimaryActorTick.bCanEverTick = false;

    // 创建炮弹的静态网格体组件（可视化炮弹模型）
    // CreateDefaultSubobject：在构造函数中创建Actor的子组件，参数为组件名称（用于编辑器识别）
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
    // 将网格体组件设为Actor的根组件（根组件决定Actor的位置/旋转/缩放，所有子组件附着到根组件）
    SetRootComponent(ProjectileMesh);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    ProjectileMesh->SetCollisionObjectType(BB_COLLISION_PROJECTILE);
    ProjectileMesh->SetCollisionResponseToAllChannels(ECR_Block);
    ProjectileMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

    // 创建炮弹移动组件（Unreal内置的弹道运动组件，自动处理抛物线、速度等物理运动）
    ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComp"));
    // 设置炮弹初始发射速度（单位：厘米/秒，Unreal默认单位为厘米）
    ProjectileMovementComp->InitialSpeed = 4000.0f;
    // 设置炮弹最大移动速度
    ProjectileMovementComp->MaxSpeed = 10000.0f;

    // 创建拖尾粒子组件（用于显示炮弹飞行时的拖尾特效）
    TrailParticles = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailParticles"));
    // 将拖尾粒子组件附着到根组件（网格体），继承根组件的位置/旋转，随炮弹同步移动
    TrailParticles->SetupAttachment(RootComponent);
    // 开启碰撞事件通知（必须！否则 OnHit 可能不会被调用）
    ProjectileMesh->SetNotifyRigidBodyCollision(true);
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (ProjectileMesh)
	{
		ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ProjectileMesh->SetCollisionObjectType(BB_COLLISION_PROJECTILE);
		ProjectileMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
	
	ProjectileMesh->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);

    // 需求1：设置6秒生命周期，无论是否有穿透buff
    SetLifeSpan(6.0f);

    //投射物生成时,产生发射音效
    if (LaunchSound) {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), LaunchSound, GetActorLocation());
    }
}

void AProjectile::OnHit(
    UPrimitiveComponent* HitComponent,    // 碰撞组件：炮弹自身的碰撞体组件
    AActor* OtherActor,                    // 其他Actor：被炮弹击中的Actor对象
    UPrimitiveComponent* OtherComp,        // 其他碰撞组件：被击中Actor的碰撞组件
    FVector NormalImpulse,                 // 法向冲量：碰撞时产生的冲击力向量
    const FHitResult& Hit                  // 命中结果：详细的碰撞检测结果数据
)
{
    // 安全检查：忽略空目标和自身
    if (!OtherActor || OtherActor == this)
    {
        Destroy();
        return;
    }

    // 获取炮弹所有者
    AActor* MyOwner = GetOwner();

    // 避免伤害自己
    if (MyOwner && OtherActor == MyOwner)
    {
        Destroy();
        return;
    }

    // ========== 阵营伤害判断 ==========
    // 检查攻击者和受害者是否都是Tank
    ATank* AttackerTank = Cast<ATank>(MyOwner);
    ATank* VictimTank = Cast<ATank>(OtherActor);
    if (AttackerTank && VictimTank)
    {
        // 获取当前游戏模式
        AGameModeBase* CurrentGM = GetWorld()->GetAuthGameMode();

        // MOBA 模式：每个玩家独立阵营，同 PlayerIndex = 同阵营，不能互相伤害
        if (Cast<ATankMOBAGameMode>(CurrentGM))
        {
            // 直接使用 Tank 的 PlayerIndex 作为阵营ID
            // MOBA 模式下每个玩家都是独立阵营（PlayerIndex 0,1,2,3 分别对应 P0,P1,P2,P3 出生点）
            int32 AttackerCamp = AttackerTank->GetPlayerIndex();
            int32 VictimCamp = VictimTank->GetPlayerIndex();

            // 只有同 PlayerIndex 才不造成伤害（自己不能打自己）
            if (AttackerCamp >= 0 && VictimCamp >= 0 && AttackerCamp == VictimCamp)
            {
                Destroy();
                return;
            }
        }
        // TeamBattle 模式：同阵营（0/2=红色，1/3=蓝色）不能互相伤害
        else if (Cast<ATeamBattleGameMode>(CurrentGM))
        {
            // 使用 PlayerIndex 判断阵营：0/2 是红色，1/3 是蓝色
            bool bAttackerIsRed = (AttackerTank->GetPlayerIndex() == 0 || AttackerTank->GetPlayerIndex() == 2);
            bool bVictimIsRed = (VictimTank->GetPlayerIndex() == 0 || VictimTank->GetPlayerIndex() == 2);

            // 同阵营不造成伤害
            if (bAttackerIsRed == bVictimIsRed)
            {
                Destroy();
                return;
            }
        }
        // 其他模式（如死斗）：不进行阵营判断，都可以互相伤害
        // 不做任何处理
    }

    // ========== 需求2：只要命中敌人就销毁（无论是否有穿透buff） ==========
    // 用 APawn 判断是否为"敌人/角色"（Tank、Tower都继承自Pawn）
    if (Cast<APawn>(OtherActor))
    {
        // 造成伤害
        if (MyOwner)
        {
            UGameplayStatics::ApplyDamage(
                OtherActor,
                Damage,
                MyOwner->GetInstigatorController(),
                MyOwner,   // ← 从 this 改成 MyOwner
                UDamageType::StaticClass()
            );
        }

        // 播放命中特效
        if (HitParticles)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), HitParticles, GetActorLocation(), GetActorRotation()
            );
        }

        if (HitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
        }

        if (HitCameraShakeClass && AttackerTank)
        {
            if (ATankPlayerController* PC = Cast<ATankPlayerController>(AttackerTank->GetController()))
            {
                PC->ClientStartCameraShake(HitCameraShakeClass);
            }
        }

        // 命中敌人：无条件销毁！
        Destroy();
        return;
    }

    // ========== 可破坏物体（如木箱）==========
    if (Cast<ADestructibleProp>(OtherActor))
    {
        // 对可破坏物体造成伤害
        if (MyOwner)
        {
            UGameplayStatics::ApplyDamage(
                OtherActor,
                Damage,
                MyOwner->GetInstigatorController(),
                MyOwner,
                UDamageType::StaticClass()
            );
        }

        // 播放命中特效
        if (HitParticles)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), HitParticles, GetActorLocation(), GetActorRotation()
            );
        }

        if (HitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
        }

        // 命中可破坏物体后销毁子弹
        Destroy();
        return;
    }

    // ========== 非敌人（如墙壁、障碍物）==========
    // 如果没有穿透buff，撞墙就销毁
    if (!bCanPierce)
    {
        if (HitParticles)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                GetWorld(), HitParticles, GetActorLocation(), GetActorRotation()
            );
        }
        if (HitSound)
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
        }
        Destroy();
        return;
    }

    // ========== 有穿透buff：穿墙继续飞 ==========
    // 如果 bCanPierce 为 true，由于频道设为 Ignore，墙壁根本不会触发 OnHit
    // 所以这里不需要写任何东西，子弹会顺着物理引擎自动飞过去
    // 这里才用到 MaxPenetrationCount
    //if (bCanPierce)
    //{
    //    ++CurrentPenetrationCount;

    //    // -1 表示无限穿透，或者未达到上限时继续飞
    //    if (MaxPenetrationCount == -1 || CurrentPenetrationCount < MaxPenetrationCount)
    //    {
    //        // 穿墙处理：把炮弹向前推一点，避免卡在墙里
    //        FVector CurrentVelocity = ProjectileMovementComp ? ProjectileMovementComp->Velocity : GetActorForwardVector() * 4000.0f;
    //        FVector Direction = CurrentVelocity.GetSafeNormal();
    //        
    //        // 将位置移到碰撞点前方
    //        SetActorLocation(Hit.ImpactPoint + Direction * 30.0f, false, nullptr, ETeleportType::TeleportPhysics);

    //        // 恢复速度继续飞
    //        if (ProjectileMovementComp)
    //        {
    //            ProjectileMovementComp->Velocity = CurrentVelocity;
    //            ProjectileMovementComp->UpdateComponentVelocity();
    //        }
    //        return; // 不要销毁，继续飞
    //    }
    //    else
    //    {
    //        // 达到穿透上限，销毁
    //        if (HitParticles)
    //        {
    //            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
    //                GetWorld(), HitParticles, GetActorLocation(), GetActorRotation()
    //            );
    //        }
    //        if (HitSound)
    //        {
    //            UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitSound, GetActorLocation());
    //        }
    //        Destroy();
    //        return;
    //    }
    //}
}

// 启用强化版视觉效果
void AProjectile::EnableBoostVisuals()
{
    // 1. 如果配置了强化版模型，就替换当前模型
    if (BoostedProjectileMesh && ProjectileMesh)
    {
        ProjectileMesh->SetStaticMesh(BoostedProjectileMesh);
    }

    // 2. 如果配置了强化版拖尾特效，就替换 Niagara 的资产
    if (BoostedTrailParticles && TrailParticles)
    {
        TrailParticles->SetAsset(BoostedTrailParticles);

        // 重新初始化粒子系统以确保立即播放新特效
        TrailParticles->ReinitializeSystem();
    }
    if (BoostedHitParticles)
    {
        HitParticles = BoostedHitParticles;
    }

    if (BoostedLaunchSound)
    {
        LaunchSound = BoostedLaunchSound;
    }

    if (BoostedHitSound)
    {
        HitSound = BoostedHitSound;
    }
}

void AProjectile::EnablePierceMode(bool bInfinitePierce, int32 InMaxPenetrationCount)
{
    bCanPierce = true;

    // 配置最大穿透目标数
    if (bInfinitePierce)
    {
        MaxPenetrationCount = -1;   // -1 表示无限
    }
    else
    {
        MaxPenetrationCount = InMaxPenetrationCount;
    }

    // 核心：忽略墙体 / 障碍物的碰撞，但仍然保留对 Pawn（坦克）的阻挡
    if (ProjectileMesh)
    {
        // 直接忽略环境和物理物体，不再触发 OnHit
        ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
        ProjectileMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
        ProjectileMesh->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore); // 加上这一行

        // 只保留对坦克的阻挡，用来计算伤害和穿透次数
        ProjectileMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }
}
void AProjectile::SetProjectileLifeSpan(float InLifeSpan)
{
    SetLifeSpan(InLifeSpan);
}
