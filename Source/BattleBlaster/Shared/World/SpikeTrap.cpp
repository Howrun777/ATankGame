#include "Shared/World/SpikeTrap.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
// 【必须包含你项目的 HealthComponent 头文件，请根据实际名字替换】
#include "Shared/Combat/HealthComponent.h" 
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

ASpikeTrap::ASpikeTrap()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(false);

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	SpikeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpikeMesh"));
	SpikeMesh->SetupAttachment(RootComp);
	SpikeMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
	DamageBox->SetupAttachment(SpikeMesh);
	DamageBox->SetBoxExtent(FVector(80.f, 80.f, 60.f));
	// 【注意】如果你想伤害所有单位，确保 DamageBox 的碰撞通道能检测到其他单位
	DamageBox->SetCollisionProfileName(TEXT("Trigger"));
	DamageBox->OnComponentBeginOverlap.AddDynamic(this, &ASpikeTrap::OnOverlapBegin);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComp);
	DetectionSphere->SetSphereRadius(2000.0f);
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 【修改】既然对所有包含 HealthComponent 的单位生效，警戒网应当可以检测大部分动态物体
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	DetectionSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
}

void ASpikeTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpikeTrap, ReplicatedState);
}

void ASpikeTrap::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		if (DetectionSphere)
		{
			DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (DamageBox)
		{
			DamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (HasAuthority() && DetectionSphere)
	{
		DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpikeTrap::OnDetectionBeginOverlap);
		DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpikeTrap::OnDetectionEndOverlap);

		// 【性能与修复核心】延迟一帧检测，确保开局站在刺上的单位一定能被扫到
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASpikeTrap::CheckInitialOverlaps);
	}

	FVector StartLoc = SpikeMesh->GetRelativeLocation();
	StartLoc.Z = ZOffsetWhenHidden;
	SpikeMesh->SetRelativeLocation(StartLoc);

	// 初始先进入休眠，等 CheckInitialOverlaps 的结果来决定是否唤醒
	if (HasAuthority())
	{
		ChangeState(ESpikeState::Dormant);
	}
	else
	{
		ApplyStateVisual();
	}
}

// 【新增】下一帧检测开局站在圈内的 Actor
void ASpikeTrap::CheckInitialOverlaps()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!DetectionSphere) return;

	TArray<AActor*> InitialActors;
	DetectionSphere->GetOverlappingActors(InitialActors);

	for (AActor* Actor : InitialActors)
	{
		// 【解耦检测】不去强转特定类，只看有没有 HealthComponent！
		if (Actor && Actor->GetComponentByClass(UHealthComponent::StaticClass()))
		{
			ActorsInDetectionRange.Add(Actor);
		}
	}

	if (ActorsInDetectionRange.Num() > 0)
	{
		ChangeState(ESpikeState::Hidden);
	}
}

void ASpikeTrap::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	// 判断进来的 Actor 有没有生命值组件
	if (OtherActor && OtherActor->GetComponentByClass(UHealthComponent::StaticClass()))
	{
		ActorsInDetectionRange.Add(OtherActor); // TSet 自动去重

		if (CurrentState == ESpikeState::Dormant)
		{
			ChangeState(ESpikeState::Hidden);
		}
	}
}

void ASpikeTrap::OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority())
	{
		return;
	}

	if (OtherActor)
	{
		// 离开时直接移出范围记录即可，不需要强制检查它是不是活的，直接踢出 Set
		ActorsInDetectionRange.Remove(OtherActor);
	}
}

void ASpikeTrap::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		UpdateClientStateVisual();
		return;
	}

	if (CurrentState == ESpikeState::Thrusting)
	{
		FVector CurrentLoc = SpikeMesh->GetRelativeLocation();
		FVector TargetLoc = FVector(CurrentLoc.X, CurrentLoc.Y, 0.0f);
		FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, ThrustSpeed);
		SpikeMesh->SetRelativeLocation(NewLoc);

		if (NewLoc.Z >= 0.0f) ChangeState(ESpikeState::Active);
	}
	else if (CurrentState == ESpikeState::Retracting)
	{
		FVector CurrentLoc = SpikeMesh->GetRelativeLocation();
		FVector TargetLoc = FVector(CurrentLoc.X, CurrentLoc.Y, ZOffsetWhenHidden);
		FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, RetractSpeed);
		SpikeMesh->SetRelativeLocation(NewLoc);

		if (NewLoc.Z <= ZOffsetWhenHidden)
		{
			// 【极其重要】：在判断前清理一波野指针 (如果在范围内死了被销毁，会导致空指针残留)
			ActorsInDetectionRange.Remove(nullptr);

			if (ActorsInDetectionRange.Num() <= 0)
			{
				ChangeState(ESpikeState::Dormant);
			}
			else
			{
				ChangeState(ESpikeState::Hidden);
			}
		}
	}
}

void ASpikeTrap::ChangeState(ESpikeState NewState)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentState = NewState;
	ReplicatedState.State = NewState;
	ReplicatedState.StateStartServerTime = GetServerTimeSeconds();
	ForceNetUpdate();

	if (CurrentState == ESpikeState::Dormant)
	{
		GetWorldTimerManager().ClearTimer(StateTimerHandle);
		SetActorTickEnabled(false);
	}
	else if (CurrentState == ESpikeState::Hidden)
	{
		DamagedActorsThisCycle.Empty();
		float ActualWaitTime = HiddenDuration + FMath::RandRange(0.0f, RandomWakeDelayMax);
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ASpikeTrap::OnHiddenTimerExpired, ActualWaitTime, false);
	}
	else if (CurrentState == ESpikeState::Thrusting)
	{
		SetActorTickEnabled(true);
		DealDamageToActors();
		if (ThrustSound) UGameplayStatics::PlaySoundAtLocation(this, ThrustSound, GetActorLocation());
	}
	else if (CurrentState == ESpikeState::Active)
	{
		SetActorTickEnabled(false);
		GetWorldTimerManager().SetTimer(StateTimerHandle, this, &ASpikeTrap::OnActiveTimerExpired, ActiveDuration, false);
	}
	else if (CurrentState == ESpikeState::Retracting)
	{
		SetActorTickEnabled(true);
	}
}

void ASpikeTrap::OnRep_ReplicatedState()
{
	const ESpikeState PreviousState = CurrentState;
	CurrentState = ReplicatedState.State;
	ApplyStateVisual();

	if (CurrentState == ESpikeState::Thrusting && PreviousState != ESpikeState::Thrusting && ThrustSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ThrustSound, GetActorLocation());
	}
}

void ASpikeTrap::ApplyStateVisual()
{
	if (!SpikeMesh)
	{
		return;
	}

	FVector NewLoc = SpikeMesh->GetRelativeLocation();
	switch (CurrentState)
	{
	case ESpikeState::Thrusting:
		if (HasAuthority())
		{
			NewLoc.Z = ZOffsetWhenHidden;
			SpikeMesh->SetRelativeLocation(NewLoc);
		}
		else
		{
			UpdateClientStateVisual();
			SetActorTickEnabled(true);
		}
		break;
	case ESpikeState::Active:
		NewLoc.Z = 0.0f;
		SpikeMesh->SetRelativeLocation(NewLoc);
		SetActorTickEnabled(false);
		break;
	case ESpikeState::Retracting:
		if (HasAuthority())
		{
			NewLoc.Z = 0.0f;
			SpikeMesh->SetRelativeLocation(NewLoc);
		}
		else
		{
			UpdateClientStateVisual();
			SetActorTickEnabled(true);
		}
		break;
	case ESpikeState::Dormant:
	case ESpikeState::Hidden:
	default:
		NewLoc.Z = ZOffsetWhenHidden;
		SpikeMesh->SetRelativeLocation(NewLoc);
		SetActorTickEnabled(false);
		break;
	}
}

void ASpikeTrap::UpdateClientStateVisual()
{
	if (HasAuthority() || !SpikeMesh)
	{
		return;
	}

	const float Duration = GetSpikeMoveDuration(CurrentState);
	if (Duration <= KINDA_SMALL_NUMBER)
	{
		SetActorTickEnabled(false);
		return;
	}

	const float Alpha = FMath::Clamp(GetReplicatedStateElapsedTime() / Duration, 0.0f, 1.0f);
	if (CurrentState == ESpikeState::Thrusting)
	{
		SetSpikeMeshRelativeZ(FMath::Lerp(ZOffsetWhenHidden, 0.0f, Alpha));
		if (Alpha >= 1.0f)
		{
			SetActorTickEnabled(false);
		}
	}
	else if (CurrentState == ESpikeState::Retracting)
	{
		SetSpikeMeshRelativeZ(FMath::Lerp(0.0f, ZOffsetWhenHidden, Alpha));
		if (Alpha >= 1.0f)
		{
			SetActorTickEnabled(false);
		}
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

void ASpikeTrap::SetSpikeMeshRelativeZ(float NewZ)
{
	if (!SpikeMesh)
	{
		return;
	}

	FVector NewLoc = SpikeMesh->GetRelativeLocation();
	NewLoc.Z = NewZ;
	SpikeMesh->SetRelativeLocation(NewLoc);
}

float ASpikeTrap::GetSpikeMoveDuration(ESpikeState State) const
{
	const float Distance = FMath::Abs(ZOffsetWhenHidden);
	if (State == ESpikeState::Thrusting)
	{
		return ThrustSpeed > KINDA_SMALL_NUMBER ? Distance / ThrustSpeed : 0.0f;
	}
	if (State == ESpikeState::Retracting)
	{
		return RetractSpeed > KINDA_SMALL_NUMBER ? Distance / RetractSpeed : 0.0f;
	}

	return 0.0f;
}

float ASpikeTrap::GetReplicatedStateElapsedTime() const
{
	return FMath::Max(0.0f, GetServerTimeSeconds() - ReplicatedState.StateStartServerTime);
}

float ASpikeTrap::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->GetServerWorldTimeSeconds() : 0.0f;
}

void ASpikeTrap::OnHiddenTimerExpired() { ChangeState(ESpikeState::Thrusting); }
void ASpikeTrap::OnActiveTimerExpired() { ChangeState(ESpikeState::Retracting); }

// 【核心修改】通用目标伤害检测
void ASpikeTrap::DealDamageToActors()
{
	if (!HasAuthority())
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	// 不再指定特定 Class，直接获取所有重叠物
	DamageBox->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		// 只有身上挂载了 HealthComponent 且这轮还没受到过伤害的目标，才去扣血
		if (Actor && Actor->GetComponentByClass(UHealthComponent::StaticClass()))
		{
			if (!DamagedActorsThisCycle.Contains(Actor))
			{
				DamagedActorsThisCycle.Add(Actor); // TSet 的 Add，O(1)
				UGameplayStatics::ApplyDamage(Actor, DamageAmount, nullptr, this, UDamageType::StaticClass());

				//UE_LOG(LogTemp, Warning, TEXT("Spike thrust hit %s for %f damage!"), *Actor->GetName(), DamageAmount);
			}
		}
	}
}

void ASpikeTrap::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentState == ESpikeState::Thrusting || CurrentState == ESpikeState::Active)
	{
		// 同理：只判断有没有生命组件
		if (OtherActor && OtherActor->GetComponentByClass(UHealthComponent::StaticClass()))
		{
			if (!DamagedActorsThisCycle.Contains(OtherActor))
			{
				DamagedActorsThisCycle.Add(OtherActor);
				UGameplayStatics::ApplyDamage(OtherActor, DamageAmount, nullptr, this, UDamageType::StaticClass());

				//UE_LOG(LogTemp, Warning, TEXT("Actor %s drove into active spikes! Took %f damage."), *OtherActor->GetName(), DamageAmount);
			}
		}
	}
}

void ASpikeTrap::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 1. 清除所有正在运行的定时器，防止 Actor 销毁后定时器回调导致空指针崩溃
	GetWorldTimerManager().ClearTimer(StateTimerHandle);

	// 2. 清空 TSet 容器，解除对所有 Actor 的引用，帮助垃圾回收 (GC)
	ActorsInDetectionRange.Empty();
	DamagedActorsThisCycle.Empty();

	// 3. 调用父类逻辑
	Super::EndPlay(EndPlayReason);
}
