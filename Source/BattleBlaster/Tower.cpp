#include "Tower.h"



#include "TankPlayerController.h"
#include "Components/SphereComponent.h" // 【新增】必须包含此头文件
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h" 
#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "HealthComponent.h" 
#include "GameFramework/GameModeBase.h"
#include "TankStageGameMode.h"


// 构造函数
ATower::ATower()
{
    // 初始化变量
    bIsDead = false;
    ActiveDeathLoopComponent = nullptr;
    ActiveRespawnComponent = nullptr;
    // 【新增】1. 创建警戒网球体组件
    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(RootComponent); // 附着到根组件
    DetectionSphere->SetSphereRadius(FireRange);     // 初始半径就是射程

    // 【新增】2. 设置碰撞属性，只对 Pawn（玩家/敌人）产生重叠响应，避免被子弹或墙壁频繁触发
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATower::BeginPlay()
{
    Super::BeginPlay();

    if (DetectionSphere)
    {
        // 1. 绑定进出警戒网的事件
        DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATower::OnDetectionSphereBeginOverlap);
        DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATower::OnDetectionSphereEndOverlap);

        // ==========================================
        // 【新增修复】 2. 开机主动扫描：检测一开始就在圈里的玩家
        // ==========================================
        TArray<AActor*> InitialOverlappingActors;
        // 这一步会直接获取当前在球体内部的所有 ATank 类的 Actor
        DetectionSphere->GetOverlappingActors(InitialOverlappingActors, ATank::StaticClass());

        for (AActor* Actor : InitialOverlappingActors)
        {
            ATank* OverlappingTank = Cast<ATank>(Actor);
            if (OverlappingTank)
            {
                // 将出生就在圈内的玩家加入追踪列表
                TargetsInRange.AddUnique(OverlappingTank);
            }
        }
    }

    // ==========================================
    // 【关键修复】 3. 绑定 HealthComponent 的生命值/死亡事件
    // ==========================================
    if (HealthComp)
    {
        HealthComp->OnHealthChanged.AddDynamic(this, &ATower::HandleTowerHealthChanged);
        HealthComp->OnDeath.AddDynamic(this, &ATower::HandleTowerDeath);
        UE_LOG(LogTemp, Warning, TEXT("Tower::BeginPlay - Health events bound successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Tower::BeginPlay - HealthComp is NULL!"));
    }

    bIsDead = false;
    SetTowerState(true);
}

// 【新增】当玩家进入警戒范围
void ATower::OnDetectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 如果进入警戒范围的 Actor 是一个 Tank
    ATank* EnteredTank = Cast<ATank>(OtherActor);
    if (EnteredTank)
    {
        // 将其加入追踪列表 (AddUnique 防止重复添加)
        TargetsInRange.AddUnique(EnteredTank);
    }
}

// 【新增】当玩家离开警戒范围
void ATower::OnDetectionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ATank* ExitedTank = Cast<ATank>(OtherActor);
    if (ExitedTank)
    {
        // 从追踪列表中移除
        TargetsInRange.Remove(ExitedTank);
    }
}

// 【重写】不再遍历全图找目标，只在警戒网里的目标中挑一个最近的
ATank* ATower::GetTargetInRange()
{
    ATank* BestTarget = nullptr;
    float MinDistance = FireRange;

    // 遍历当前在警戒网里的坦克（通常只有 1 个，最多几个）
    for (int32 i = TargetsInRange.Num() - 1; i >= 0; --i)
    {
        ATank* CurrentTank = TargetsInRange[i];

        // 核心判断：坦克必须有效、且活着
        if (CurrentTank && CurrentTank->IsAlive)
        {
            float Distance = FVector::Dist(GetActorLocation(), CurrentTank->GetActorLocation());
            if (Distance <= MinDistance)
            {
                MinDistance = Distance;
                BestTarget = CurrentTank;
            }
        }
        else if (!CurrentTank)
        {
            // 如果指针为空（比如坦克被 Destroy 了），顺手清理掉
            TargetsInRange.RemoveAt(i);
        }
    }

    return BestTarget;
}

void ATower::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    ATank* Target = GetTargetInRange();
    if (Target)
    {
        RotateTurret(Target->GetActorLocation());

        if (!IsTargetBlocked(Target))
        {
            Fire();
        }
    }
    else
    {
        // 没有目标时重置冷却，一旦玩家进入范围立刻开火
        Fire_LastTime = 0.0f;
    }
}

void ATower::Fire()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();

    if (CurrentTime - Fire_LastTime < FireRate) return;

    if (ProjectileSpawnPoint)
    {
        FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
        FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

        AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
        if (Projectile)
        {
            Projectile->SetOwner(this);
            Fire_LastTime = CurrentTime;
        }
    }
}


// 检查目标是否被障碍物阻挡（子弹路径上有其他物体）
bool ATower::IsTargetBlocked(ATank* Target)
{
    if (!Target || !ProjectileSpawnPoint)
    {
        return true;
    }

    FVector StartLocation = ProjectileSpawnPoint->GetComponentLocation();
    FVector EndLocation = Target->GetActorLocation();

    // 射线检测参数
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this); // 忽略 Tower 自身
    QueryParams.AddIgnoredActor(Target); // 忽略目标 Tank

    // 从炮塔位置到目标位置发射射线
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility, // 使用可视通道，可以根据需要调整
        QueryParams
    );

    // 如果有碰撞，说明路径被阻挡
    if (bHit)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Tower: Fire path blocked by %s"), *HitResult.GetActor()->GetName());
        return true;
    }

    return false;
}

// =========================================================
// 核心逻辑：处理防御塔死亡/复活流程
// =========================================================
void ATower::HandleDestruction()
{
    // 【新增】防御塔被打黑时，立刻清空当前的追踪目标，防止死后还企图开火
    TargetsInRange.Empty();

    Super::HandleDestruction();

    UE_LOG(LogTemp, Display, TEXT("Tower HandleDestruction! Starts Death Sequence."));

    // 1. 标记为死亡状态
    bIsDead = true;

    // 2. 进入“假死”状态（隐藏模型，关闭碰撞）
    SetTowerState(false);

    // 3. 【立即播放特效 1】 (DeathLoopEffect) 并保存引用
    if (DeathLoopEffect)
    {
        // SpawnSystemAtLocation 返回一个 UNiagaraComponent*，我们可以存下来用于以后关闭它
        ActiveDeathLoopComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            DeathLoopEffect,
            GetActorLocation()+FVector(0.0f,0.0f,-50.0f),
            GetActorRotation()
        );
        UE_LOG(LogTemp, Log, TEXT("Tower: Playing Death Loop Effect (1)."));
    }

    // 4. 设置【正式复活】定时器 (60秒后触发)
    // 单人闯关模式下，禁止敌人复活
    AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(this);
    bool bIsSinglePlayerMode = CurrentGM && CurrentGM->IsA(ATankStageGameMode::StaticClass());
    if (!bIsSinglePlayerMode)
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_RespawnRevive,
            this,
            &ATower::ReviveTower,
            RespawnTotalTime,
            false
        );
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("Tower: TankStageGameMode - Respawn disabled for this tower."));
    }
}

// 辅助函数：控制塔的显隐和碰撞
void ATower::SetTowerState(bool bActive)
{
    // 控制整个 Actor 的显隐
    SetActorHiddenInGame(!bActive);

    // 控制 Actor 的碰撞
    SetActorEnableCollision(bActive);

    // 额外确保胶囊体碰撞状态正确
    UCapsuleComponent* Capsule = FindComponentByClass<UCapsuleComponent>();
    if (Capsule)
    {
        if (bActive)
        {
            Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        }
        else
        {
            Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
}

// 步骤 1: 正式复活
void ATower::ReviveTower()
{
    UE_LOG(LogTemp, Log, TEXT("Tower Reviving..."));

    // 1. 【停止特效 1】
    if (ActiveDeathLoopComponent)
    {
        ActiveDeathLoopComponent->Deactivate(); // 或者 DestroyComponent()
        ActiveDeathLoopComponent = nullptr;     // 清空指针
        UE_LOG(LogTemp, Log, TEXT("Tower: Stopped Death Loop Effect (1)."));
    }

    // 2. 恢复状态 (显示模型，开启碰撞)
    SetTowerState(true);
    bIsDead = false;

    // 3. 满血复活
    if (HealthComp)
    {
        // 确保你的 HealthComponent 里写了 ResetHealth 函数
        HealthComp->ResetHealth();
    }

    // 4. 【播放特效 2】 (RespawnSuccessEffect) 并保存引用
    if (RespawnSuccessEffect)
    {
        ActiveRespawnComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            RespawnSuccessEffect,
            GetActorLocation() + FVector(0.0f, 0.0f, -50.0f),
            GetActorRotation()
        );
        UE_LOG(LogTemp, Log, TEXT("Tower: Playing Respawn Effect (2)."));
    }

    // 5. 设置定时器：3秒后停止特效 2
    GetWorldTimerManager().SetTimer(
        TimerHandle_StopRespawnFx,
        this,
        &ATower::StopRespawnEffect,
        RespawnEffectDuration, // 3.0f
        false
    );
}

// 步骤 2: 3秒后停止复活特效
void ATower::StopRespawnEffect()
{
    if (ActiveRespawnComponent)
    {
        ActiveRespawnComponent->Deactivate();
        ActiveRespawnComponent = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Tower: Stopped Respawn Effect (2) after 3 seconds."));
    }
}

void ATower::ApplyDifficultyMultiplier(float Multiplier)
{
    CurrentDifficultyMultiplier = Multiplier;

    if (HealthComp)
    {
        float OldMaxHealth = HealthComp->MaxHealth;
        float HealthPercent = (OldMaxHealth > 0) ? (HealthComp->CurrentHealth / OldMaxHealth) : 1.0f;

        HealthComp->MaxHealth = OldMaxHealth * Multiplier;
        HealthComp->CurrentHealth = HealthComp->MaxHealth * HealthPercent;
    }

    // 射程变远
    FireRange *= Multiplier;

    // 【关键新增】难度提高导致射程变远时，警戒网的半径也要同步扩大！
    if (DetectionSphere)
    {
        DetectionSphere->SetSphereRadius(FireRange);
    }

    // 攻速变快
    FireRate /= Multiplier;

    UE_LOG(LogTemp, Display, TEXT("Tower difficulty applied: Multiplier=%.2f, FireRange=%.0f, FireRate=%.2f"),
        Multiplier, FireRange, FireRate);
}

// =========================================================
// HealthComponent 事件处理：血量变化时触发（可选：受伤特效等）
// =========================================================
void ATower::HandleTowerHealthChanged(
    UHealthComponent* InHealthComp,
    float Health,
    float HealthDelta,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    // 这里可以添加受击特效、受击震动等
}

// =========================================================
// HealthComponent 事件处理：死亡时触发 → 调用 HandleDestruction
// =========================================================
void ATower::HandleTowerDeath(
    UHealthComponent* InHealthComp,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    UE_LOG(LogTemp, Warning, TEXT("Tower::HandleTowerDeath - Tower is dying!"));
    HandleDestruction();
    //增加对击杀者控制器控制的Tank的奖励
    if (ATankPlayerController* KillerPC = Cast<ATankPlayerController>(InstigatedBy))
    {
        if (APawn* KillerPawn = KillerPC->GetPawn())
        {
            if (ATank* KillerTank = Cast<ATank>(KillerPawn))
            {
                KillerTank->HandleKillReward();
            }
        }
    }
}