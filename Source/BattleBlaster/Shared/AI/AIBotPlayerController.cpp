// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shared/AI/AIBotPlayerController.h"
#include "Shared/Pawns/Tank.h"
#include "Shared/Pawns/NPC/Tower.h"
#include "Modes/MOBA/Turret.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/State/TankPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Navigation/PathFollowingComponent.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Modes/TeamBattle/TeamBattleGameMode.h"

bool AAIBotPlayerController::IsEnemy(AActor* Target) const
{
    if (!Target || !ControlledTank)
    {
        return false;
    }

    // 只对 Tank 做阵营判定；Tower 等其它目标按“敌对”处理
    const ATank* MyTank = ControlledTank;
    const ATank* TargetTank = Cast<ATank>(Target);
    if (!TargetTank)
    {
        return true;
    }

    const ATeamBattleGameMode* TeamGM = GetWorld() ? Cast<ATeamBattleGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
    if (!TeamGM)
    {
        // 没有团队 GameMode 就按非团队处理
        return true;
    }

    // 团队模式下，如果 PlayerIndex 还没初始化好，宁可先不攻击（避免误打队友）
    if (MyTank->GetPlayerIndex() < 0 || TargetTank->GetPlayerIndex() < 0)
    {
        return false;
    }

    return !TeamGM->IsSameCamp(MyTank->GetPlayerIndex(), TargetTank->GetPlayerIndex());
}

bool AAIBotPlayerController::PassesFilter(AActor* Actor) const
{
    if (!Actor) return false;
    for (const TSubclassOf<AActor>& FilterClass : AttackFilterTypes)
    {
        if (Actor->IsA(FilterClass))
        {
            return true;
        }
    }
    return false;
}
AAIBotPlayerController::AAIBotPlayerController()
{
    // 【核心修复】：强制让虚幻引擎为这个 AI 生成一个 PlayerState！
    // 这样 GameMode 就会把它当成一个“正式玩家”来分配分数和 KDA
    bWantsPlayerState = true;

    // 攻击列表默认值：Tank, Tower, Turret
    AttackFilterTypes.Add(ATank::StaticClass());
    AttackFilterTypes.Add(ATower::StaticClass());
    AttackFilterTypes.Add(ATurret::StaticClass());
}
void AAIBotPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // AI 成功附身新复活的坦克时，刷新控制指针
    ControlledTank = Cast<ATank>(InPawn);
    if (ControlledTank)
    {
        ResetAIState();

        // 【关键修复】：复活时清空攻击列表！旧列表里可能还有死掉 Tank 的悬垂指针
        // 下一帧 RefreshTargetFromAttackList 计时器会重新填充正确的目标
        AttackTargetList.Empty();

        // 给新复活的坦克重新应用难度配置（比如移动速度加成）
        ApplyDifficultySettings(CurrentDifficulty);
    }
}

void AAIBotPlayerController::OnUnPossess()
{
    Super::OnUnPossess();

    // 坦克死亡脱离控制时，清空指针并终止指令
    ControlledTank = nullptr;
    ResetAIState();
    StopMovement();
}
void AAIBotPlayerController::BeginPlay()
{
    Super::BeginPlay();
    // 1. 初始化控制的 Tank (提前获取，避免后续用的时候是 null)
    ControlledTank = Cast<ATank>(GetPawn());

    // 2. 根据设置的难度档位，覆盖 AI 的默认参数
    ApplyDifficultySettings(CurrentDifficulty);

    // 初始设置
    NextDirectionChangeTime = GetWorld()->GetTimeSeconds() + FMath::RandRange(0.5f, DirectionChangeInterval);
    NextFireTime = GetWorld()->GetTimeSeconds() + FMath::RandRange(1.0f, FireInterval);

    // 获取控制的 Tank
    ControlledTank = Cast<ATank>(GetPawn());

    // 初始化攻击列表：遍历所有 Tank 和 Tower，符合条件的才加入
    TArray<AActor*> AllTanks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);
    for (AActor* TankActor : AllTanks)
    {
        ATank* PlayerTank = Cast<ATank>(TankActor);
        if (PlayerTank && PlayerTank->IsAlive && PlayerTank != ControlledTank)
        {
            if (PassesFilter(PlayerTank) && IsEnemy(PlayerTank))
            {
                AttackTargetList.AddUnique(PlayerTank);
            }
        }
    }

    // 【关键修复】：初始化时也加入存活的敌方 Tower！否则 AI 开局就看不到 Tower
    TArray<AActor*> AllTowers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATower::StaticClass(), AllTowers);
    for (AActor* TowerActor : AllTowers)
    {
        ATower* EnemyTower = Cast<ATower>(TowerActor);
        if (!EnemyTower) continue;

        UHealthComponent* HealthComp = EnemyTower->FindComponentByClass<UHealthComponent>();
        float CurrentHealth = HealthComp ? HealthComp->CurrentHealth : 0.0f;
        if (CurrentHealth <= 0.0f) continue;

        AttackTargetList.AddUnique(EnemyTower);
    }

    // 初始化战术参数
    CurrentTacticalMove = ETacticalMoveType::None;
    PreviousTargetLocation = FVector::ZeroVector;
    PreviousTargetUpdateTime = 0.0f;
    EstimatedTargetVelocity = FVector::ZeroVector;
    AggressionLevel = 0.0f;

    // 固定频率刷新目标：每 0.5 秒从攻击列表选择最近敌人
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            TargetQueryTimerHandle,
            this,
            &AAIBotPlayerController::RefreshTargetFromAttackList,
            TargetQueryInterval,
            true,
            0.2f
        );
    }
}
void AAIBotPlayerController::ApplyDifficultySettings(EAIDifficulty NewDifficulty)
{
    CurrentDifficulty = NewDifficulty;

    switch (CurrentDifficulty)
    {
    case EAIDifficulty::Easy:
        // 【简单档】：又慢又瞎，基本是活靶子
        AIFireRate = 2.5f;                  // 攻速：极慢 (2.5秒开一炮)
        ChaseSpeedScale = 0.4f;             // 移速：只有玩家 40%
        AimErrorAngle = 15.0f;              // 精准度：极差 (±15度误差)
        bEnablePredictiveAiming = false;    // 预测瞄准：关闭 (只会打你上一秒的位置)
        DodgeChance = 0.1f;                 // 闪避概率：10% (基本不躲)
        bEnableTacticalMovement = false;    // 战术机动：关闭 (只会直线走)
        StateChangeCooldown = 2.5f;         // 反应速度：迟钝 (2.5秒才改变一次主意)
        TargetQueryInterval = 1.0f;         // 索敌频率：慢 (1秒扫一次目标)
        break;

    case EAIDifficulty::Normal:
        // 【普通档】：标准的 AI，有正常的拉扯和瞄准
        AIFireRate = 1.2f;                  // 攻速：正常
        ChaseSpeedScale = 0.75f;            // 移速：玩家的 75%
        AimErrorAngle = 5.0f;               // 精准度：有小幅误差
        bEnablePredictiveAiming = true;     // 预测瞄准：开启
        PredictionTime = 0.3f;              // 预测提前量：正常
        DodgeChance = 0.4f;                 // 闪避概率：40%
        bEnableTacticalMovement = true;     // 战术机动：开启 (会侧滑/环绕)
        StateChangeCooldown = 1.5f;         // 反应速度：正常 (1.5秒更新一次状态)
        TargetQueryInterval = 0.5f;         // 索敌频率：正常 (0.5秒)
        break;

    case EAIDifficulty::Hard:
        // 【困难档】：压迫感强，火力猛，走位灵活
        AIFireRate = 0.6f;                  // 攻速：快
        ChaseSpeedScale = 0.9f;             // 移速：玩家的 90%
        AimErrorAngle = 1.0f;               // 精准度：很高 (几乎是指哪打哪)
        bEnablePredictiveAiming = true;
        PredictionTime = 0.5f;              // 预测提前量：大 (会疯狂打你提前量)
        DodgeChance = 0.75f;                // 闪避概率：75% (很难打中它)
        bEnableTacticalMovement = true;
        StrafeSpeed = 250.0f;               // 环绕速度变快
        StateChangeCooldown = 0.5f;         // 反应速度：极快 (0.5秒就能从追击变逃跑)
        TargetQueryInterval = 0.2f;         // 索敌频率：极快
        AggressiveHealthThreshold = 0.5f;   // 更容易进入激进状态
        break;

    case EAIDifficulty::Insane:
        // 【噩梦档】：不当人的数值怪，无上限突破
        AIFireRate = 0.15f;                 // 攻速：机枪坦克！(0.15秒一发)
        ChaseSpeedScale = 1.0f;             // 移速：与玩家完全一致(最大)
        AimErrorAngle = 0.0f;               // 精准度：完美自瞄 (0误差)
        bEnablePredictiveAiming = true;
        PredictionTime = 0.4f;              // 根据你的子弹飞行速度可以微调
        DodgeChance = 1.0f;                 // 闪避概率：100% (检测到子弹必触发闪避指令)
        bEnableTacticalMovement = true;
        StrafeSpeed = 400.0f;               // 像泥鳅一样滑
        StateChangeCooldown = 0.1f;         // 反应速度：瞬间反应
        TargetQueryInterval = 0.1f;         // 索敌频率：实时雷达
        AggressiveHealthThreshold = 0.0f;   // 永远是疯狗激进模式
        FleeHealthThreshold = 0.0f;         // 死战不退 (绝不逃跑)
        break;
    }

    // --- 同步应用需要传导到底层系统的参数 ---

    // 1. 如果你在 Tank 类里有改变最大速度的方法，这里需要调用它。
    // 假设你的 Tank 有类似 SetMaxSpeed 的函数（如果没有，可以不管，目前只影响 NavMesh）
    if (ControlledTank)
    {
        // ControlledTank->GetMovementComponent()->MaxSpeed = BaseSpeed * ChaseSpeedScale; 
    }

    // 2. 刷新索敌定时器 (以适配新的 TargetQueryInterval)
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(TargetQueryTimerHandle);
        GetWorld()->GetTimerManager().SetTimer(
            TargetQueryTimerHandle,
            this,
            &AAIBotPlayerController::RefreshTargetFromAttackList,
            TargetQueryInterval,
            true,
            0.1f
        );
    }
}
void AAIBotPlayerController::RefreshTargetFromAttackList()
{
    // 【核心修复】：双重确认，保证拿到当前最新的活体坦克
    ATank* CurrentPawn = Cast<ATank>(GetPawn());
    if (ControlledTank != CurrentPawn)
    {
        ControlledTank = CurrentPawn;
    }
    // 坦克死掉或者没拿稳前，不允许进行索敌
    if (!ControlledTank || !ControlledTank->IsAlive) return;
    // 清理无效目标（死亡/销毁/同队 Tank）
    for (int32 i = AttackTargetList.Num() - 1; i >= 0; --i)
    {
        AActor* TargetActor = AttackTargetList[i];
        if (!TargetActor)
        {
            AttackTargetList.RemoveAtSwap(i);
            continue;
        }

        // 同队 Tank 直接移除（理论上不会进来，但防御一下）
        if (!IsEnemy(TargetActor))
        {
            AttackTargetList.RemoveAtSwap(i);
            continue;
        }

        if (ATank* TargetTank = Cast<ATank>(TargetActor))
        {
            if (!TargetTank->IsAlive)
            {
                AttackTargetList.RemoveAtSwap(i);
            }
            continue;
        }

        if (ATower* TargetTower = Cast<ATower>(TargetActor))
        {
            UHealthComponent* HealthComp = TargetTower->FindComponentByClass<UHealthComponent>();
            const float CurrentHealth = HealthComp ? HealthComp->CurrentHealth : 0.0f;
            if (CurrentHealth <= 0.0f)
            {
                AttackTargetList.RemoveAtSwap(i);
            }
        }
    }

    // 如果攻击列表被清空了，确保重新加入当前场景中所有敌方 Tank
    if (AttackTargetList.Num() == 0)
    {
        TArray<AActor*> AllTanks;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);

        for (AActor* TankActor : AllTanks)
        {
            ATank* EnemyTank = Cast<ATank>(TankActor);
            if (!EnemyTank || EnemyTank == ControlledTank) continue;
            if (!EnemyTank->IsAlive) continue;

            // 仅添加敌对阵营的坦克，保证“攻击数组一直有敌人 Tank 的坐标”
            if (IsEnemy(EnemyTank))
            {
                AttackTargetList.AddUnique(EnemyTank);
            }
        }
    }

    AActor* NewTarget = SelectNearestTargetFromList();
    if (NewTarget)
    {
        // 直接切换并追击
        if (NewTarget != CurrentTarget)
        {
            SetTarget(NewTarget);
        }
    }
    else
    {
        // 列表里没有可打的目标就停止追击，进入巡逻
        if (CurrentTarget)
        {
            StopChasing();
        }
    }
}

// ================= 攻击列表相关函数 =================

void AAIBotPlayerController::AddTargetToAttackList(AActor* NewTarget)
{
    if (!NewTarget) return;
    if (!PassesFilter(NewTarget)) return;
    if (!IsEnemy(NewTarget)) return;
    AttackTargetList.AddUnique(NewTarget);
    
    // 更新最后威胁方向
    if (ControlledTank)
    {
        LastKnownThreatDirection = (NewTarget->GetActorLocation() - ControlledTank->GetActorLocation()).GetSafeNormal();
        LastThreatUpdateTime = GetWorld()->GetTimeSeconds();
    }
}

void AAIBotPlayerController::RemoveTargetFromAttackList(AActor* Target)
{
    AttackTargetList.Remove(Target);
}

void AAIBotPlayerController::OnAttackedBy(AActor* Attacker)
{
    if (!Attacker)
    {
        return;
    }
    // 【核心修复】：双重确认
    ATank* CurrentPawn = Cast<ATank>(GetPawn());
    if (ControlledTank != CurrentPawn)
    {
        ControlledTank = CurrentPawn;
    }

    // 确保已经拿到自己控制的坦克引用
    if (!ControlledTank)
    {
        ControlledTank = Cast<ATank>(GetPawn());
    }

    // 死亡后就不用还手了
    if (!ControlledTank || !ControlledTank->IsAlive)
    {
        return;
    }

    // 团队模式：不反击队友（避免追着队友打）
    if (!IsEnemy(Attacker))
    {
        return;
    }

    // 【新增过滤】限制只能追踪/攻击过滤器类型内的 Actor（Tank, Tower, Turret等）
    if (!PassesFilter(Attacker))
    {
        return;
    }

    // 将攻击者加入攻击列表
    AddTargetToAttackList(Attacker);

    UE_LOG(LogTemp, Warning, TEXT("AI Tank was attacked by %s, adding to attack list!"), *Attacker->GetName());

    // ==================== 核心：优先还手攻击者 ====================
    // 1. 如果当前没有目标，直接把攻击者设为新目标
    if (!CurrentTarget)
    {
        SetTarget(Attacker);
        return;
    }

    // 2. 如果当前目标就是攻击者，就不用切换
    if (CurrentTarget == Attacker)
    {
        return;
    }

    // 3. 如果攻击者比当前目标更近，或者你希望永远优先反击攻击者，可以直接切换目标
    const FVector MyLoc = ControlledTank->GetActorLocation();
    const float DistToAttackerSq = FVector::DistSquared(MyLoc, Attacker->GetActorLocation());
    const float DistToCurrentSq = FVector::DistSquared(MyLoc, CurrentTarget->GetActorLocation());

    // 这里简单处理：只要攻击者离自己不比当前目标远太多，就切过去还手
    // 你也可以改成"总是优先反击攻击者"：直接调用 SetTarget(Attacker);
    if (DistToAttackerSq <= DistToCurrentSq * 1.2f)
    {
        SetTarget(Attacker);
    }
}

AActor* AAIBotPlayerController::SelectNearestTargetFromList()
{
    if (!ControlledTank || AttackTargetList.IsEmpty()) return nullptr;

    const FVector MyLoc = ControlledTank->GetActorLocation();
    AActor* NearestTarget = nullptr;
    float MinDistSq = FLT_MAX;

    // 从攻击列表中找出最近的目标
    for (AActor* TargetActor : AttackTargetList)
    {
        if (!TargetActor) continue;
        if (!IsEnemy(TargetActor)) continue;

        // 检查目标是否仍然有效（Tank要活着，Tower要有血）
        ATank* TargetTank = Cast<ATank>(TargetActor);
        if (TargetTank)
        {
            if (!TargetTank->IsAlive) continue;
        }
        else
        {
            // 尝试作为 Tower 处理
            ATower* TargetTower = Cast<ATower>(TargetActor);
            if (TargetTower)
            {
                UHealthComponent* HealthComp = TargetTower->FindComponentByClass<UHealthComponent>();
                float CurrentHealth = HealthComp ? HealthComp->CurrentHealth : 0.0f;
                if (CurrentHealth <= 0.0f) continue;
            }
        }

        const float DistSq = FVector::DistSquared(MyLoc, TargetActor->GetActorLocation());
        if (DistSq < MinDistSq)
        {
            MinDistSq = DistSq;
            NearestTarget = TargetActor;
        }
    }

    return NearestTarget;
}

// ================= 威胁评估系统 =================

float AAIBotPlayerController::CalculateTargetThreat(AActor* Target)
{
    if (!Target || !ControlledTank) return 0.0f;
    if (!IsEnemy(Target)) return 0.0f;

    float Threat = 0.0f;

    // 1. 距离威胁：越近威胁越高
    float Dist = FVector::Distance(ControlledTank->GetActorLocation(), Target->GetActorLocation());
    float DistanceThreat = FMath::Clamp(1.0f - (Dist / 2000.0f), 0.0f, 1.0f);
    Threat += DistanceThreat * 40.0f;

    // 2. 血量威胁：敌人血量越少威胁越低（好欺负）
    ATank* TargetTank = Cast<ATank>(Target);
    if (TargetTank)
    {
        UHealthComponent* HealthComp = TargetTank->FindComponentByClass<UHealthComponent>();
        if (HealthComp && HealthComp->MaxHealth > 0.0f)
        {
            float HealthPercent = HealthComp->CurrentHealth / HealthComp->MaxHealth;
            Threat += (1.0f - HealthPercent) * 20.0f; // 敌人血少，我们威胁就高
        }
    }

    // 3. 最近攻击过AI的威胁更高
    if (Target)
    {
        if (ATankPlayerState* PS = ControlledTank->GetPlayerState<ATankPlayerState>())
        {
            for (const FAttackerRecord& Record : PS->GetAttackerQueue())
            {
                if (Record.AttackerTank.Get() == Target)
                {
                    Threat += 30.0f;
                    break;
                }
            }
        }
    }

    return Threat;
}

AActor* AAIBotPlayerController::SelectBestTarget()
{
    if (!ControlledTank) return nullptr;

    // 如果有多个目标，根据威胁程度选择
    if (AttackTargetList.Num() > 1)
    {
        AActor* BestTarget = nullptr;
        float HighestThreat = -FLT_MAX;

        for (AActor* Target : AttackTargetList)
        {
            if (!Target) continue;
            if (!IsEnemy(Target)) continue;

            float Threat = CalculateTargetThreat(Target);

            // 优先打低血量敌人
            if (bPrioritizeLowHealthTargets)
            {
                ATank* TargetTank = Cast<ATank>(Target);
                if (TargetTank)
                {
                    UHealthComponent* HealthComp = TargetTank->FindComponentByClass<UHealthComponent>();
                    if (HealthComp && HealthComp->MaxHealth > 0.0f)
                    {
                        float HealthPercent = HealthComp->CurrentHealth / HealthComp->MaxHealth;
                        if (HealthPercent < 0.3f)
                        {
                            Threat += 25.0f; // 优先击杀残血
                        }
                    }
                }
            }

            if (Threat > HighestThreat)
            {
                HighestThreat = Threat;
                BestTarget = Target;
            }
        }

        return BestTarget;
    }

    // 否则返回最近的
    return SelectNearestTargetFromList();
}

// ================= 原有函数（修改为使用攻击列表） =================

AActor* AAIBotPlayerController::FindNearestTarget()
{
    // 优先使用高级目标选择
    AActor* BestTarget = SelectBestTarget();
    if (BestTarget) return BestTarget;

    // 如果列表为空或所有目标都失效，才搜索全图
    if (!ControlledTank) return nullptr;

    const FVector MyLoc = ControlledTank->GetActorLocation();
    AActor* NearestTarget = nullptr;
    float MinDistSq = FLT_MAX;

    // 1) 搜索所有 Tank，找出最近的敌方 Tank
    TArray<AActor*> AllTanks;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATank::StaticClass(), AllTanks);

    for (AActor* TankActor : AllTanks)
    {
        ATank* EnemyTank = Cast<ATank>(TankActor);
        if (EnemyTank && EnemyTank != ControlledTank && EnemyTank->IsAlive)
        {
            if (!IsEnemy(EnemyTank)) continue;
            const float DistSq = FVector::DistSquared(MyLoc, EnemyTank->GetActorLocation());
            if (DistSq < MinDistSq)
            {
                MinDistSq = DistSq;
                NearestTarget = EnemyTank;
            }
        }
    }

    // 2) 搜索所有 Tower，找出最近的敌方 Tower
    TArray<AActor*> AllTowers;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATower::StaticClass(), AllTowers);

    for (AActor* TowerActor : AllTowers)
    {
        ATower* EnemyTower = Cast<ATower>(TowerActor);
        if (EnemyTower)
        {
            UHealthComponent* HealthComp = EnemyTower->FindComponentByClass<UHealthComponent>();
            float CurrentHealth = HealthComp ? HealthComp->CurrentHealth : 0.0f;
            if (CurrentHealth > 0.0f)
            {
                const float DistSq = FVector::DistSquared(MyLoc, EnemyTower->GetActorLocation());
                if (DistSq < MinDistSq)
                {
                    MinDistSq = DistSq;
                    NearestTarget = EnemyTower;
                }
            }
        }
    }

    return NearestTarget;
}

void AAIBotPlayerController::SetTarget(AActor* NewTarget)
{
    if (NewTarget && !IsEnemy(NewTarget))
    {
        return;
    }
    CurrentTarget = NewTarget;
    if (NewTarget)
    {
        // 添加到攻击列表
        AddTargetToAttackList(NewTarget);
        StartChase(NewTarget);

        // 重置目标速度估算
        PreviousTargetLocation = NewTarget->GetActorLocation();
        PreviousTargetUpdateTime = GetWorld()->GetTimeSeconds();
        EstimatedTargetVelocity = FVector::Zero();
    }
    else
    {
        StopChasing();
    }
}

void AAIBotPlayerController::StartChase(AActor* Target)
{
    if (!Target || !ControlledTank) return;

    bIsChasing = true;

    // 使用 MoveToActor 进行 NavMesh 寻路
    MoveToActor(
        Target,
        StopChaseDistance,      // 停止距离
        true,                   // bStopOnOverlap - 到达目标时停止
        true,                   // bCanStrafe - 允许侧面移动
        true                    // bAllowPartialPath - 允许部分路径
    );
}

void AAIBotPlayerController::StopChasing()
{
    bIsChasing = false;
    StopMovement();
    CurrentTarget = nullptr;
    CurrentCombatState = EAICombatState::Idle;
}

void AAIBotPlayerController::ResetAIState()
{
    bIsChasing = false;
    CurrentTarget = nullptr;
    CurrentCombatState = EAICombatState::Idle;
}

void AAIBotPlayerController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    // 目标选择由 0.5 秒定时器统一负责，这里只做“追击状态复位”
    bIsChasing = false;
}

// ================= 核心：Tick 主循环 =================

void AAIBotPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 【核心修复】：双重确保我们获取到的是 GameMode 刚刚分配给 AI 的"新坦克"
    ATank* CurrentPawn = Cast<ATank>(GetPawn());
    if (ControlledTank != CurrentPawn)
    {
        ControlledTank = CurrentPawn;
        if (ControlledTank)
        {
            // 如果成功切换成了新坦克，将状态重置（从逃跑或者追击变回清醒待机）
            ResetAIState();
        }
    }

    // 死亡隔离拦截：如果处于死亡状态或没有身体，AI 大脑强行停止（避免对旧指针发号施令引起发呆BUG）
    if (!ControlledTank || !ControlledTank->IsAlive)
    {
        return;
    }

    // 绝对保护：目标无效或变成队友，立刻打断并置空
    if (CurrentTarget)
    {
        if (!CurrentTarget->IsValidLowLevel() || !IsEnemy(CurrentTarget))
        {
            RemoveTargetFromAttackList(CurrentTarget);
            StopChasing();
        }
    }

    UpdateCombatState();
    ExecuteCurrentState(DeltaTime);

    if (CurrentTarget && ControlledTank->IsAlive &&
        CurrentCombatState != EAICombatState::Flee &&
        CurrentCombatState != EAICombatState::Idle)
    {
        RotateTurretTowardsEnemy();
        AttemptFire();
    }
}

// ================= 状态机系统 =================

void AAIBotPlayerController::UpdateCombatState()
{
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // 冷却时间内不切换状态
    if (CurrentTime - LastStateChangeTime < StateChangeCooldown)
    {
        return;
    }

    if (!ControlledTank || !CurrentTarget)
    {
        CurrentCombatState = EAICombatState::Idle;
        return;
    }

    // 获取AI自身血量百分比
    UHealthComponent* AIHealthComp = ControlledTank->FindComponentByClass<UHealthComponent>();
    float AIHealthPercent = 1.0f;
    if (AIHealthComp && AIHealthComp->MaxHealth > 0.0f)
    {
        AIHealthPercent = AIHealthComp->CurrentHealth / AIHealthComp->MaxHealth;
    }

    // 获取与目标的距离
    float DistToTarget = GetDistanceToTarget();

    // 状态转换逻辑
    EAICombatState NewState = CurrentCombatState;

    // 逃跑判定（血量过低）
    if (ShouldFlee())
    {
        NewState = EAICombatState::Flee;
    }
    // 激进追击（血量高且距离远）
    else if (AIHealthPercent >= AggressiveHealthThreshold && DistToTarget > AttackRange)
    {
        NewState = EAICombatState::Chase;
    }
    // 保持距离（中等距离，游击战）
    else if (DistToTarget > MinKeepDistance && DistToTarget < MaxKeepDistance)
    {
        // 随机选择环绕或保持距离
        if (FMath::RandRange(0.0f, 1.0f) > 0.5f)
        {
            NewState = EAICombatState::Strafe;
        }
        else
        {
            NewState = EAICombatState::KeepDistance;
        }
    }
    // 距离太近，选择环绕
    else if (DistToTarget <= MinKeepDistance)
    {
        NewState = EAICombatState::Strafe;
    }
    // 距离太远，追击
    else if (DistToTarget > MaxKeepDistance)
    {
        NewState = EAICombatState::Chase;
    }
    // 激进模式下更倾向追击
    else if (AggressionLevel > 0.6f && DistToTarget <= AttackRange)
    {
        NewState = EAICombatState::Chase;
    }

    if (NewState != CurrentCombatState)
    {
        CurrentCombatState = NewState;
        LastStateChangeTime = CurrentTime;
        UE_LOG(LogTemp, Warning, TEXT("AI State changed to: %d"), (int32)CurrentCombatState);
    }
}

void AAIBotPlayerController::ExecuteCurrentState(float DeltaTime)
{
    if (!ControlledTank) return;

    // === 独立处理 Idle（无目标巡逻）状态，不需要 CurrentTarget ===
    if (CurrentCombatState == EAICombatState::Idle)
    {
        if (GetWorld()->GetTimeSeconds() >= NextDirectionChangeTime)
        {
            // 你原本写的巡逻逻辑
            float MoveForward = FMath::RandRange(-0.5f, 0.5f);
            float MoveRight = FMath::RandRange(-0.5f, 0.5f);
            ControlledTank->MoveWithAI(MoveForward, MoveRight);
            NextDirectionChangeTime = GetWorld()->GetTimeSeconds() + DirectionChangeInterval;
        }
        return; // 处理完 Idle 直接返回
    }

    // === 对于其他战斗状态，如果没有目标，直接拦截返回 ===
    if (!CurrentTarget) return;

    const float DistToTarget = GetDistanceToTarget();

    switch (CurrentCombatState)
    {
    case EAICombatState::Chase:
        // 如果你的地图没做 NavMesh，这里 MoveToActor 不会生效！
        // 建议检查是否铺了 NavMeshBoundsVolume
        if (DistToTarget > StopChaseDistance && !bIsChasing)
        {
            StartChase(CurrentTarget);
        }
        else if (DistToTarget <= StopChaseDistance && bIsChasing)
        {
            StopMovement();
            bIsChasing = false;
        }
        break;

    case EAICombatState::Strafe:
        if (bEnableTacticalMovement) ExecuteTacticalMovement(DeltaTime);
        break;

    case EAICombatState::KeepDistance:
        if (DistToTarget > MaxKeepDistance)
        {
            StartChase(CurrentTarget);
        }
        else if (DistToTarget < MinKeepDistance)
        {
            FVector AwayDir = (ControlledTank->GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
            ControlledTank->MoveWithAI(AwayDir.X, AwayDir.Y);
        }
        else
        {
            if (bEnableTacticalMovement) ExecuteTacticalMovement(DeltaTime);
        }
        break;

    case EAICombatState::Flee:
    {
        FVector AwayDir = (ControlledTank->GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
        ControlledTank->MoveWithAI(AwayDir.X * 1.5f, AwayDir.Y * 1.5f);
    }
    break;

    case EAICombatState::TakeCover:
    {
        FVector AwayDir = (ControlledTank->GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();
        ControlledTank->MoveWithAI(AwayDir.X, AwayDir.Y);
    }
    break;

    case EAICombatState::Ambush:
        StopMovement();
        break;

    default:
        break;
    }
}

// ================= 战术机动系统 =================

void AAIBotPlayerController::ExecuteTacticalMovement(float DeltaTime)
{
    if (!bEnableTacticalMovement || !ControlledTank || !CurrentTarget) return;

    // 选择新的战术移动
    if (CurrentTacticalMove == ETacticalMoveType::None ||
        CurrentTacticalMoveTime >= TacticalMoveDuration)
    {
        CurrentTacticalMove = ChooseTacticalMove(DeltaTime);
        CurrentTacticalMoveTime = 0.0f;
        TacticalMoveDuration = FMath::RandRange(1.5f, 3.0f);
    }

    // 执行当前战术移动
    switch (CurrentTacticalMove)
    {
    case ETacticalMoveType::CircleLeft:
    case ETacticalMoveType::CircleRight:
        PerformCircleMovement(CurrentTacticalMove, DeltaTime);
        break;

    case ETacticalMoveType::ForwardStrafe:
    case ETacticalMoveType::BackwardStrafe:
    case ETacticalMoveType::RandomStrafe:
        PerformStrafeMovement(CurrentTacticalMove, DeltaTime);
        break;

    case ETacticalMoveType::None:
    default:
        break;
    }

    CurrentTacticalMoveTime += DeltaTime;
}

ETacticalMoveType AAIBotPlayerController::ChooseTacticalMove(float DeltaTime)
{
    float DistToTarget = GetDistanceToTarget();

    // 根据距离选择战术
    if (DistToTarget < 200.0f)
    {
        // 太近了，快速绕开
        return FMath::RandRange(0, 1) ? ETacticalMoveType::CircleLeft : ETacticalMoveType::CircleRight;
    }
    else if (DistToTarget < 400.0f)
    {
        // 中距离，环绕
        int32 Rand = FMath::RandRange(0, 2);
        if (Rand == 0) return ETacticalMoveType::CircleLeft;
        if (Rand == 1) return ETacticalMoveType::CircleRight;
        return ETacticalMoveType::RandomStrafe;
    }
    else
    {
        // 远距离，侧滑接近
        int32 Rand = FMath::RandRange(0, 2);
        if (Rand == 0) return ETacticalMoveType::ForwardStrafe;
        if (Rand == 1) return ETacticalMoveType::RandomStrafe;
        return ETacticalMoveType::CircleLeft;
    }
}

void AAIBotPlayerController::PerformCircleMovement(ETacticalMoveType CircleDir, float DeltaTime)
{
    if (!ControlledTank || !CurrentTarget) return;

    FVector TankLoc = ControlledTank->GetActorLocation();
    FVector TargetLoc = CurrentTarget->GetActorLocation();

    // 计算从目标到AI的方向
    FVector ToAI = (TankLoc - TargetLoc).GetSafeNormal();

    // 计算切线方向（环绕方向）
    FVector RightVector = FVector::CrossProduct(FVector::UpVector, ToAI).GetSafeNormal();

    // 根据环绕方向选择左右
    if (CircleDir == ETacticalMoveType::CircleLeft)
    {
        RightVector = -RightVector;
    }

    // 归一化并应用速度
    FVector MoveDir = RightVector.GetSafeNormal();
    FVector2D MoveInput = FVector2D(MoveDir.X, MoveDir.Y);

    // 同时向圆心方向微调，保持环绕半径
    float DistToTarget = GetDistanceToTarget();
    if (DistToTarget > StrafeRadius)
    {
        // 太远了，稍微向目标方向移动
        MoveInput.X += ToAI.X * 0.3f;
        MoveInput.Y += ToAI.Y * 0.3f;
    }
    else if (DistToTarget < StrafeRadius * 0.7f)
    {
        // 太近了，稍微远离目标
        MoveInput.X -= ToAI.X * 0.3f;
        MoveInput.Y -= ToAI.Y * 0.3f;
    }

    ControlledTank->MoveWithAI(MoveInput.GetSafeNormal().X, MoveInput.GetSafeNormal().Y);
}

void AAIBotPlayerController::PerformStrafeMovement(ETacticalMoveType StrafeType, float DeltaTime)
{
    if (!ControlledTank || !CurrentTarget) return;

    FVector TankLoc = ControlledTank->GetActorLocation();
    FVector TargetLoc = CurrentTarget->GetActorLocation();

    // 计算到目标的方向
    FVector ToTarget = (TargetLoc - TankLoc).GetSafeNormal();
    FVector RightVector = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();

    FVector2D MoveInput;

    switch (StrafeType)
    {
    case ETacticalMoveType::ForwardStrafe:
        // 前进并侧滑
        MoveInput = FVector2D(ToTarget.X * 0.7f + RightVector.X * 0.3f,
                              ToTarget.Y * 0.7f + RightVector.Y * 0.3f);
        break;

    case ETacticalMoveType::BackwardStrafe:
        // 后退并侧滑
        MoveInput = FVector2D(-ToTarget.X * 0.5f + RightVector.X * 0.5f,
                              -ToTarget.Y * 0.5f + RightVector.Y * 0.5f);
        break;

    case ETacticalMoveType::RandomStrafe:
    default:
        // 随机选择左右侧滑
        {
            bool bStrafeRight = FMath::RandRange(0, 1) == 1;
            MoveInput = FVector2D(ToTarget.X * 0.3f + (bStrafeRight ? RightVector.X : -RightVector.X) * 0.7f,
                                  ToTarget.Y * 0.3f + (bStrafeRight ? RightVector.Y : -RightVector.Y) * 0.7f);
        }
        break;
    }

    ControlledTank->MoveWithAI(MoveInput.GetSafeNormal().X, MoveInput.GetSafeNormal().Y);
}

// ================= 闪避系统 =================

bool AAIBotPlayerController::ShouldDodge()
{
    if (!bEnableDodge || !ControlledTank) return false;

    // 检测是否有子弹接近（简化版：基于最近攻击时间）
    float TimeSinceLastHit = GetWorld()->GetTimeSeconds() - ControlledTank->LastHitTime;

    // 如果最近0.5秒内被击中，有概率闪避
    if (TimeSinceLastHit < 0.5f)
    {
        // 连续闪避会增加逃跑倾向
        if (GetWorld()->GetTimeSeconds() - LastDodgeTime > 2.0f)
        {
            ConsecutiveDodgeCount = 0;
        }

        float AdjustedDodgeChance = FMath::Clamp(DodgeChance + (ConsecutiveDodgeCount * 0.1f), 0.0f, 0.95f);
        return FMath::RandRange(0.0f, 1.0f) < AdjustedDodgeChance;
    }

    return false;
}

FVector AAIBotPlayerController::CalculateDodgeDirection()
{
    if (!ControlledTank || !CurrentTarget) return FVector::ZeroVector;

    FVector TankLoc = ControlledTank->GetActorLocation();
    FVector TargetLoc = CurrentTarget->GetActorLocation();

    // 从目标到AI的方向
    FVector FromTarget = (TankLoc - TargetLoc).GetSafeNormal();

    // 随机选择左/右闪避
    FVector RightVector = FVector::CrossProduct(FVector::UpVector, FromTarget).GetSafeNormal();
    if (FMath::RandRange(0, 1) == 0)
    {
        RightVector = -RightVector;
    }

    // 稍微向后闪避
    return (RightVector - FromTarget * 0.3f).GetSafeNormal();
}

void AAIBotPlayerController::ExecuteDodge()
{
    if (!ControlledTank) return;

    FVector DodgeDir = CalculateDodgeDirection();

    // 执行闪避移动
    ControlledTank->MoveWithAI(DodgeDir.X * 2.0f, DodgeDir.Y * 2.0f);

    ConsecutiveDodgeCount++;
    LastDodgeTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Warning, TEXT("AI executed dodge!"));
}

// ================= 血量感知系统 =================

bool AAIBotPlayerController::ShouldFlee() const
{
    if (!ControlledTank) return false;

    UHealthComponent* HealthComp = ControlledTank->FindComponentByClass<UHealthComponent>();
    if (!HealthComp) return false;

    float HealthPercent = 1.0f;
    if (HealthComp && HealthComp->MaxHealth > 0.0f)
    {
        HealthPercent = HealthComp->CurrentHealth / HealthComp->MaxHealth;
    }

    // 血量低于阈值时逃跑
    if (HealthPercent < FleeHealthThreshold)
    {
        return true;
    }

    // 连续闪避多次后也考虑逃跑
    if (ConsecutiveDodgeCount >= 3)
    {
        return true;
    }

    return false;
}

bool AAIBotPlayerController::ShouldBeAggressive() const
{
    if (!ControlledTank) return false;

    UHealthComponent* HealthComp = ControlledTank->FindComponentByClass<UHealthComponent>();
    if (!HealthComp) return false;

    float HealthPercent = 1.0f;
    if (HealthComp && HealthComp->MaxHealth > 0.0f)
    {
        HealthPercent = HealthComp->CurrentHealth / HealthComp->MaxHealth;
    }
    return HealthPercent >= AggressiveHealthThreshold;
}

FVector AAIBotPlayerController::GetFleeDestination()
{
    if (!ControlledTank || !CurrentTarget) return FVector::ZeroVector;

    // 简单策略：向地图中心或远离敌人的方向跑
    FVector AwayFromEnemy = (ControlledTank->GetActorLocation() - CurrentTarget->GetActorLocation()).GetSafeNormal();

    // 简单实现：向敌人相反方向跑
    return ControlledTank->GetActorLocation() + AwayFromEnemy * 500.0f;
}

// ================= 预测瞄准系统 =================

FVector AAIBotPlayerController::CalculatePredictedAimPoint()
{
    if (!CurrentTarget) return FVector::ZeroVector;

    // 基于目标当前速度和预测时间计算未来位置
    FVector PredictedLocation = CurrentTarget->GetActorLocation() + EstimatedTargetVelocity * PredictionTime;

    // 加上一些随机偏移，模拟人类的不完美瞄准
    if (AimErrorAngle > 0.0f)
    {
        float RandomAngle = FMath::RandRange(-AimErrorAngle * 0.5f, AimErrorAngle * 0.5f);
        FRotator ErrorRot = FRotator(0.0f, RandomAngle, 0.0f);
        PredictedLocation = PredictedLocation + ErrorRot.RotateVector(FVector(1.0f, 0.0f, 0.0f)) * 30.0f;
    }

    return PredictedLocation;
}

bool AAIBotPlayerController::IsAimingAtTarget() const
{
    if (!ControlledTank || !CurrentTarget) return false;

    // 获取炮塔朝向
    FRotator TurretRot = ControlledTank->GetActorRotation();
    FVector TurretForward = TurretRot.Vector();

    // 获取到目标的方向
    FVector ToTarget = (CurrentTarget->GetActorLocation() - ControlledTank->GetActorLocation()).GetSafeNormal();

    // 计算角度差
    float DotProduct = FVector::DotProduct(TurretForward, ToTarget);
    float Angle = FMath::Acos(DotProduct);

    // 如果角度小于5度，认为瞄准了
    return FMath::RadiansToDegrees(Angle) < 5.0f;
}

// ================= 辅助函数 =================

float AAIBotPlayerController::GetDistanceToTarget() const
{
    if (!ControlledTank || !CurrentTarget) return FLT_MAX;
    return FVector::Distance(ControlledTank->GetActorLocation(), CurrentTarget->GetActorLocation());
}

float AAIBotPlayerController::GetAngleToTarget() const
{
    if (!ControlledTank || !CurrentTarget) return 180.0f;

    FVector ToTarget = (CurrentTarget->GetActorLocation() - ControlledTank->GetActorLocation()).GetSafeNormal();
    FVector Forward = ControlledTank->GetActorForwardVector();

    float Dot = FVector::DotProduct(Forward, ToTarget);
    return FMath::Acos(Dot);
}

bool AAIBotPlayerController::HasLineOfSightToTarget()
{
    if (!ControlledTank || !CurrentTarget) return false;

    // 从炮管发射点开始检测，而不是从坦克中心
    FVector Start;
    USceneComponent* SpawnPoint = ControlledTank->ProjectileSpawnPoint;
    if (SpawnPoint)
    {
        Start = SpawnPoint->GetComponentLocation();
    }
    else
    {
        Start = ControlledTank->GetActorLocation();
    }
    
    FVector End = CurrentTarget->GetActorLocation();

    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(ControlledTank);
    Params.AddIgnoredActor(CurrentTarget);

    bool bHasLineOfSight = !GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    
    // 如果没有直线视野，可以输出日志用于调试
    if (!bHasLineOfSight)
    {
        UE_LOG(LogTemp, Verbose, TEXT("AI: Fire path blocked by %s"), *Hit.GetActor()->GetName());
    }
    
    return bHasLineOfSight;
}

// ================= 瞄准与开火 =================

void AAIBotPlayerController::RotateTurretTowardsEnemy()
{
    if (!ControlledTank || !CurrentTarget) return;

    // 获取目标位置
    FVector TargetPos;

    if (bEnablePredictiveAiming)
    {
        // 使用预测瞄准
        TargetPos = CalculatePredictedAimPoint();
    }
    else
    {
        TargetPos = CurrentTarget->GetActorLocation();
    }

    // 添加随机误差
    if (AimErrorAngle > 0.0f)
    {
        float RandomAngle = FMath::RandRange(-AimErrorAngle, AimErrorAngle);
        FRotator ErrorRot = FRotator(0.0f, RandomAngle, 0.0f);
        TargetPos = TargetPos + ErrorRot.RotateVector(FVector(1.0f, 0.0f, 0.0f)) * 50.0f;
    }

    // 调用 Tank 的炮塔转向函数
    ControlledTank->RotateTurret(TargetPos);
}

void AAIBotPlayerController::AttemptFire()
{
    if (!ControlledTank || !CurrentTarget || !ControlledTank->IsAlive) return;

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    const float DistToTarget = GetDistanceToTarget();

    if (DistToTarget <= AttackRange && CurrentTime >= NextFireTime)
    {
        // 检查射击路径上是否有遮挡
        if (!HasLineOfSightToTarget())
        {
            // 路径被阻挡，不开火
            return;
        }
        
        // 开火
        ControlledTank->Fire();

        // 移除底层的 0.3 秒硬限制，改成 0.05 秒（防止除零或死循环即可）
        float FireRateVariance = AIFireRate * FMath::RandRange(-0.1f, 0.1f); // 减小一点随机波动
        NextFireTime = CurrentTime + FMath::Max(0.05f, AIFireRate + FireRateVariance);
    }
}
