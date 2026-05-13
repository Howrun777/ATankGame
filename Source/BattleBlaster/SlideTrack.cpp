#include "SlideTrack.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "Tank.h"
#include "Components/SphereComponent.h" 
#include "GameFramework/FloatingPawnMovement.h"

ASlideTrack::ASlideTrack()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 【优化】默认关闭 Tick，玩家来了再开

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComp"));
	RootComponent = BoxComp;
	BoxComp->SetBoxExtent(FVector(100.0f, 100.0f, 20.0f));
	// 建议在蓝图中专门设置一个碰撞通道，这里默认使用OverlapAllDynamic
	BoxComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrackParticleComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrackParticleComp"));
	TrackParticleComp->SetupAttachment(RootComponent);

	BoxComp->OnComponentBeginOverlap.AddDynamic(this, &ASlideTrack::OnOverlapBegin);
	BoxComp->OnComponentEndOverlap.AddDynamic(this, &ASlideTrack::OnOverlapEnd);
	// 【新增】警戒网初始化
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(3000.0f); // 设置一个较大的唤醒范围（比如 30 米）
	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DetectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ASlideTrack::BeginPlay()
{
	Super::BeginPlay();

	// 绑定警戒网事件
	if (DetectionSphere)
	{
		DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASlideTrack::OnDetectionBeginOverlap);
		DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ASlideTrack::OnDetectionEndOverlap);

		// 【性能与修复核心】
		// 不要在这里直接 GetOverlappingActors，因为 BeginPlay 时物理碰撞树可能尚未就绪！
		// 使用 TimerManager 延迟到下一帧(NextTick)执行，确保所有 Actor 的碰撞已在世界中注册完毕。
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ASlideTrack::CheckInitialOverlaps);
	}

	CurrentState = ESlideTrackState::State1_Speed;
	// 【随机化】给初始状态一个随机倒计时，防止所有滑道同时变色
	CurrentStateRemainingTime = bEnableStateSwitch ? FMath::RandRange(0.5f, State1Duration) : State1Duration;

	UpdateMeshAndMaterial();
}
// 【新增】下一帧执行的初始化检测逻辑
void ASlideTrack::CheckInitialOverlaps()
{
	if (!DetectionSphere) return;

	TArray<AActor*> InitialActors;
	DetectionSphere->GetOverlappingActors(InitialActors, ATank::StaticClass());

	// 遍历并加入 TSet
	for (AActor* Actor : InitialActors)
	{
		ATank* Tank = Cast<ATank>(Actor);
		if (Tank)
		{
			PlayersInDetectionRange.Add(Tank); // TSet 会自动忽略重复添加
		}
	}

	// 如果有玩家在范围内，立刻唤醒 Tick
	if (PlayersInDetectionRange.Num() > 0)
	{
		SetActorTickEnabled(true);
	}
}
// 【优化】玩家靠近，唤醒滑道
void ASlideTrack::OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ATank* Tank = Cast<ATank>(OtherActor);
	if (Tank)
	{
		// 使用 TSet 防止引擎诡异的多重 Overlap 触发导致的计数 BUG
		PlayersInDetectionRange.Add(Tank);

		// 只要人数大于 0 就开启（因为默认 Tick 是关的，频繁调 true 开销极小，引擎底层有过滤）
		if (PlayersInDetectionRange.Num() > 0)
		{
			SetActorTickEnabled(true);
		}
	}
}

// 【优化】玩家离开，休眠滑道
void ASlideTrack::OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ATank* Tank = Cast<ATank>(OtherActor);
	if (Tank)
	{
		// 移除该玩家
		PlayersInDetectionRange.Remove(Tank);

		// 只有当警戒范围内真的没有玩家时，彻底关闭 Tick 以节省性能
		if (PlayersInDetectionRange.Num() == 0)
		{
			SetActorTickEnabled(false);
		}
	}
}
void ASlideTrack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 动画逻辑：让滑道缓慢自转
	AddActorLocalRotation(FRotator(0.0f, RotateSpeed * DeltaTime, 0.0f));

	// 只处理状态切换定时器，坦克的持续效果完全交由Overlap事件处理
	if (bEnableStateSwitch)
	{
		CurrentStateRemainingTime -= DeltaTime;
		if (CurrentStateRemainingTime <= 0.0f)
		{
			SwitchToNextState();
		}
	}
}

void ASlideTrack::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ATank* PlayerTank = Cast<ATank>(OtherActor);
	if (PlayerTank)
	{
		// 无论如何，将其加入数组（AddUnique 会自动处理去重）
		TanksOnTrack.AddUnique(PlayerTank);

		// 直接应用当前状态的速度加成
		ApplySpeedEffect(PlayerTank, GetCurrentStateSpeedMultiplier());
	}
}

void ASlideTrack::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ATank* PlayerTank = Cast<ATank>(OtherActor);
	if (PlayerTank)
	{
		// 判断当前Actor是否完全离开了当前滑道的 BoxComp
		if (!BoxComp->IsOverlappingActor(OtherActor))
		{
			// 从当前滑道的记录列表中移除
			TanksOnTrack.Remove(PlayerTank);

			// 【修复重叠BUG的核心逻辑】：检查坦克是否还在其他滑道上
			TArray<AActor*> OverlappingTracks;
			// 获取与坦克重叠的所有 ASlideTrack 类型的 Actor
			PlayerTank->GetOverlappingActors(OverlappingTracks, ASlideTrack::StaticClass());

			bool bIsOnAnotherTrack = false;
			ASlideTrack* OtherTrackToApply = nullptr;

			for (AActor* Actor : OverlappingTracks)
			{
				ASlideTrack* OtherTrack = Cast<ASlideTrack>(Actor);
				// 确保找到的不是自己（因为引擎可能因为判定延迟把当前滑道也包含进去）
				if (OtherTrack && OtherTrack != this)
				{
					// 确保坦克真的在另一个滑道的碰撞框内
					if (OtherTrack->BoxComp->IsOverlappingActor(PlayerTank))
					{
						bIsOnAnotherTrack = true;
						OtherTrackToApply = OtherTrack;
						break; // 只要找到一个就可以跳出循环
					}
				}
			}

			// 如果坦克还在别的滑道上
			if (bIsOnAnotherTrack && OtherTrackToApply)
			{
				// 不要移除速度！而是让另一个滑道立刻重新应用它的速度加成（无缝衔接）
				OtherTrackToApply->ApplySpeedEffect(PlayerTank, OtherTrackToApply->GetCurrentStateSpeedMultiplier());
				//UE_LOG(LogTemp, Warning, TEXT("SlideTrack: Tank left this track but is on another. Transferred speed control."));
			}
			else
			{
				// 只有当坦克真正离开了所有滑道，才移除速度效果
				RemoveSpeedEffect(PlayerTank);
			}
		}
	}
}

void ASlideTrack::ApplySpeedEffect(ATank* Tank, float SpeedMultiplier)
{
	if (!Tank) return;

	// 获取基础速度
	float BaseSpd = Tank->BaseSpeed;
	if (BaseSpd <= 0.0f)
	{
		BaseSpd = Tank->Speed; // 容错处理
	}

	// 应用速度倍率
	Tank->Speed = BaseSpd * SpeedMultiplier;

	// 【修复点】：转换为 UFloatingPawnMovement 才能访问 MaxSpeed
	UFloatingPawnMovement* MovementComp = Cast<UFloatingPawnMovement>(Tank->GetMovementComponent());
	if (MovementComp)
	{
		MovementComp->MaxSpeed = Tank->Speed;
	}

	//UE_LOG(LogTemp, Warning, TEXT("SlideTrack: Applied speed effect %.2fx. New speed: %.2f"), SpeedMultiplier, Tank->Speed);
}

void ASlideTrack::RemoveSpeedEffect(ATank* Tank)
{
	if (!Tank) return;

	// 恢复坦克的原始基础速度
	float BaseSpd = Tank->BaseSpeed;
	if (BaseSpd > 0.0f)
	{
		Tank->Speed = BaseSpd;
	}

	// 【修复点】：转换为 UFloatingPawnMovement 才能访问 MaxSpeed
	UFloatingPawnMovement* MovementComp = Cast<UFloatingPawnMovement>(Tank->GetMovementComponent());
	if (MovementComp)
	{
		MovementComp->MaxSpeed = Tank->Speed;
	}

	//UE_LOG(LogTemp, Warning, TEXT("SlideTrack: Removed speed effect. Restored speed: %.2f"), Tank->Speed);
}

void ASlideTrack::SwitchToNextState()
{
	if (CurrentState == ESlideTrackState::State1_Speed)
	{
		CurrentState = ESlideTrackState::State2_Slow;
		CurrentStateRemainingTime = State2Duration;
	}
	else
	{
		CurrentState = ESlideTrackState::State1_Speed;
		CurrentStateRemainingTime = State1Duration;
	}

	UpdateMeshAndMaterial();

	// 【重要】：当状态突然切换时，直接遍历更新仍在滑道上的所有Tank的速度
	float NewMultiplier = GetCurrentStateSpeedMultiplier();

	// 【修改】：使用倒序遍历 TArray，这样即使在循环中删除了元素，也不会导致数组越界崩溃
	for (int32 i = TanksOnTrack.Num() - 1; i >= 0; --i)
	{
		ATank* Tank = TanksOnTrack[i];
		if (Tank) // 如果 Tank 不是 nullptr (没有被销毁)
		{
			ApplySpeedEffect(Tank, NewMultiplier);
		}
		else
		{
			// 如果坦克已经被销毁了(被GC变成了nullptr)，从数组中安全移除
			TanksOnTrack.RemoveAt(i);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("SlideTrack: Switched to state %d"), (int32)CurrentState);
}

void ASlideTrack::UpdateMeshAndMaterial()
{
	if (CurrentState == ESlideTrackState::State1_Speed)
	{
		if (State1Mesh) MeshComp->SetStaticMesh(State1Mesh);
		if (State1Material) MeshComp->SetMaterial(0, State1Material);
	}
	else
	{
		if (State2Mesh) MeshComp->SetStaticMesh(State2Mesh);
		if (State2Material) MeshComp->SetMaterial(0, State2Material);
	}
}

float ASlideTrack::GetCurrentStateDuration() const
{
	return (CurrentState == ESlideTrackState::State1_Speed) ? State1Duration : State2Duration;
}

float ASlideTrack::GetCurrentStateSpeedMultiplier() const
{
	return (CurrentState == ESlideTrackState::State1_Speed) ? State1SpeedMultiplier : State2SpeedMultiplier;
}