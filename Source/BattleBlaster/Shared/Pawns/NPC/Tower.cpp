#include "Shared/Pawns/NPC/Tower.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Stage/TankStageGameMode.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Controllers/TankPlayerController.h"

ATower::ATower()
{
    bReplicates = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(20.0f);
    SetMinNetUpdateFrequency(5.0f);
    PrimaryActorTick.bCanEverTick = true;

    bIsDead = false;
    ActiveDeathLoopComponent = nullptr;
    ActiveRespawnComponent = nullptr;

    DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(RootComponent);
    DetectionSphere->SetSphereRadius(FireRange);
    DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ATower::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATower, bIsDead);
    DOREPLIFETIME(ATower, ReplicatedTurretRotation);
}

void ATower::BeginPlay()
{
    Super::BeginPlay();

    if (DetectionSphere)
    {
        if (HasAuthority())
        {
            DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATower::OnDetectionSphereBeginOverlap);
            DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ATower::OnDetectionSphereEndOverlap);

            TArray<AActor*> InitialOverlappingActors;
            DetectionSphere->GetOverlappingActors(InitialOverlappingActors, ATank::StaticClass());
            for (AActor* Actor : InitialOverlappingActors)
            {
                if (ATank* OverlappingTank = Cast<ATank>(Actor))
                {
                    TargetsInRange.AddUnique(OverlappingTank);
                }
            }
        }
        else
        {
            DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }

    if (HealthComp)
    {
        if (HasAuthority())
        {
            HealthComp->OnHealthChanged.AddDynamic(this, &ATower::HandleTowerHealthChanged);
            HealthComp->OnDeath.AddDynamic(this, &ATower::HandleTowerDeath);
            UE_LOG(LogTemp, Warning, TEXT("Tower::BeginPlay - Health events bound successfully"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Tower::BeginPlay - HealthComp is NULL!"));
    }

    if (HasAuthority())
    {
        bIsDead = false;
        ReplicatedTurretRotation = TurretMesh ? TurretMesh->GetComponentRotation() : GetActorRotation();
    }

    SetTowerState(!bIsDead);
}

void ATower::OnDetectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    if (ATank* EnteredTank = Cast<ATank>(OtherActor))
    {
        TargetsInRange.AddUnique(EnteredTank);
    }
}

void ATower::OnDetectionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority())
    {
        return;
    }

    if (ATank* ExitedTank = Cast<ATank>(OtherActor))
    {
        TargetsInRange.Remove(ExitedTank);
    }
}

ATank* ATower::GetTargetInRange()
{
    if (bIsDead)
    {
        return nullptr;
    }

    ATank* BestTarget = nullptr;
    float MinDistance = FireRange;

    for (int32 i = TargetsInRange.Num() - 1; i >= 0; --i)
    {
        ATank* CurrentTank = TargetsInRange[i];
        if (!IsValid(CurrentTank))
        {
            TargetsInRange.RemoveAt(i);
            continue;
        }

        if (!CurrentTank->GetIsAlive())
        {
            continue;
        }

        const float Distance = FVector::Dist(GetActorLocation(), CurrentTank->GetActorLocation());
        if (Distance <= MinDistance)
        {
            MinDistance = Distance;
            BestTarget = CurrentTank;
        }
    }

    return BestTarget;
}

void ATower::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    ATank* Target = GetTargetInRange();
    if (Target)
    {
        RotateTurret(Target->GetActorLocation());
        if (TurretMesh)
        {
            ReplicatedTurretRotation = TurretMesh->GetComponentRotation();
        }

        if (!IsTargetBlocked(Target))
        {
            Fire();
        }
    }
    else
    {
        Fire_LastTime = 0.0f;
    }
}

void ATower::Fire()
{
    if (!HasAuthority() || bIsDead || !ProjectileClass || !ProjectileSpawnPoint)
    {
        return;
    }

    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - Fire_LastTime < FireRate)
    {
        return;
    }

    const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
    const FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = this;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
    if (Projectile)
    {
        Projectile->SetOwner(this);
        Projectile->SetInstigator(this);
        Fire_LastTime = CurrentTime;
    }
}

bool ATower::IsTargetBlocked(ATank* Target)
{
    if (!Target || !ProjectileSpawnPoint)
    {
        return true;
    }

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(Target);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        ProjectileSpawnPoint->GetComponentLocation(),
        Target->GetActorLocation(),
        ECC_Visibility,
        QueryParams
    );

    return bHit;
}

void ATower::HandleDestruction()
{
    if (!HasAuthority() || bIsDead)
    {
        return;
    }

    TargetsInRange.Empty();
    Super::HandleDestruction();

    UE_LOG(LogTemp, Display, TEXT("Tower HandleDestruction! Starts Death Sequence."));

    bIsDead = true;
    SetTowerState(false);
    ForceNetUpdate();
    StartDeathLoopEffect();

    AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(this);
    const bool bIsSinglePlayerMode = CurrentGM && CurrentGM->IsA(ATankStageGameMode::StaticClass());
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

void ATower::OnRep_IsDead()
{
    TargetsInRange.Empty();

    if (bIsDead)
    {
        Super::HandleDestruction();
        SetTowerState(false);
        StartDeathLoopEffect();
        return;
    }

    StopDeathLoopEffect();
    SetTowerState(true);
    StartRespawnEffect();
}

void ATower::OnRep_TurretRotation()
{
    if (TurretMesh && !bIsDead)
    {
        TurretMesh->SetWorldRotation(ReplicatedTurretRotation);
    }
}

void ATower::SetTowerState(bool bActive)
{
    SetActorHiddenInGame(!bActive);
    SetActorEnableCollision(bActive);
    SetActorTickEnabled(bActive && HasAuthority());

    if (!bActive)
    {
        TargetsInRange.Empty();
    }

    if (UCapsuleComponent* Capsule = FindComponentByClass<UCapsuleComponent>())
    {
        Capsule->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    }

    if (DetectionSphere)
    {
        DetectionSphere->SetCollisionEnabled((bActive && HasAuthority()) ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    }
}

void ATower::ReviveTower()
{
    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Tower Reviving..."));

    StopDeathLoopEffect();
    SetTowerState(true);
    bIsDead = false;

    if (HealthComp)
    {
        HealthComp->ResetHealth();
    }

    ForceNetUpdate();
    StartRespawnEffect();
}

void ATower::StopRespawnEffect()
{
    if (ActiveRespawnComponent)
    {
        ActiveRespawnComponent->Deactivate();
        ActiveRespawnComponent = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Tower: Stopped Respawn Effect."));
    }
}

void ATower::StartDeathLoopEffect()
{
    if (!DeathLoopEffect || ActiveDeathLoopComponent)
    {
        return;
    }

    ActiveDeathLoopComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        this,
        DeathLoopEffect,
        GetActorLocation() + FVector(0.0f, 0.0f, -50.0f),
        GetActorRotation()
    );
    UE_LOG(LogTemp, Log, TEXT("Tower: Playing Death Loop Effect."));
}

void ATower::StopDeathLoopEffect()
{
    if (ActiveDeathLoopComponent)
    {
        ActiveDeathLoopComponent->Deactivate();
        ActiveDeathLoopComponent = nullptr;
        UE_LOG(LogTemp, Log, TEXT("Tower: Stopped Death Loop Effect."));
    }
}

void ATower::StartRespawnEffect()
{
    if (RespawnSuccessEffect)
    {
        ActiveRespawnComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            this,
            RespawnSuccessEffect,
            GetActorLocation() + FVector(0.0f, 0.0f, -50.0f),
            GetActorRotation()
        );
        UE_LOG(LogTemp, Log, TEXT("Tower: Playing Respawn Effect."));
    }

    if (RespawnEffectDuration > 0.0f)
    {
        GetWorldTimerManager().SetTimer(
            TimerHandle_StopRespawnFx,
            this,
            &ATower::StopRespawnEffect,
            RespawnEffectDuration,
            false
        );
    }
}

void ATower::ApplyDifficultyMultiplier(float Multiplier)
{
    CurrentDifficultyMultiplier = Multiplier;

    if (HealthComp)
    {
        const float OldMaxHealth = HealthComp->MaxHealth;
        const float HealthPercent = OldMaxHealth > 0.0f ? HealthComp->CurrentHealth / OldMaxHealth : 1.0f;

        HealthComp->MaxHealth = OldMaxHealth * Multiplier;
        HealthComp->CurrentHealth = HealthComp->MaxHealth * HealthPercent;
    }

    FireRange *= Multiplier;
    if (DetectionSphere)
    {
        DetectionSphere->SetSphereRadius(FireRange);
    }

    FireRate /= Multiplier;

    UE_LOG(LogTemp, Display, TEXT("Tower difficulty applied: Multiplier=%.2f, FireRange=%.0f, FireRate=%.2f"),
        Multiplier, FireRange, FireRate);
}

void ATower::HandleTowerHealthChanged(
    UHealthComponent* InHealthComp,
    float Health,
    float HealthDelta,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
}

void ATower::HandleTowerDeath(
    UHealthComponent* InHealthComp,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    if (!HasAuthority())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Tower::HandleTowerDeath - Tower is dying!"));
    HandleDestruction();

    if (ATankPlayerController* KillerPC = Cast<ATankPlayerController>(InstigatedBy))
    {
        if (ATank* KillerTank = Cast<ATank>(KillerPC->GetPawn()))
        {
            KillerTank->HandleKillReward();
        }
    }
}
