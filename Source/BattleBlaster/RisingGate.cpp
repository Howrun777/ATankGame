#include "RisingGate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Tank.h"

ARisingGate::ARisingGate()
{
	PrimaryActorTick.bCanEverTick = true;

	// 1. 根组件
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	// 2. 闸门模型
	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(RootComp);
	GateMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 3. 触发区域盒子（用于检测玩家接近）
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComp);
	TriggerBox->SetBoxExtent(FVector(TriggerDistance, TriggerDistance, 100.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ARisingGate::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ARisingGate::OnOverlapEnd);
}

void ARisingGate::BeginPlay()
{
	Super::BeginPlay();

	// 记录初始位置
	InitialLocation = GateMesh->GetRelativeLocation();

	// 初始状态：闸门处于最低位置
	CurrentHeight = 0.0f;
	ChangeState(EGateState::Lowered);

	// 开始定时检测玩家距离
	bIsPlayerInTrigger = false;
	GetWorldTimerManager().SetTimer(CheckTimerHandle, this, &ARisingGate::CheckPlayerProximity, 0.1f, true);
}

void ARisingGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 处理闸门的上升和下降动画
	if (CurrentState == EGateState::Rising)
	{
		// 目标是升起指定高度
		FVector CurrentLoc = GateMesh->GetRelativeLocation();
		FVector TargetLoc = FVector(CurrentLoc.X, CurrentLoc.Y, InitialLocation.Z + RaisedHeight);

		// 匀速向上移动
		FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, RiseSpeed);
		GateMesh->SetRelativeLocation(NewLoc);

		// 更新当前高度
		CurrentHeight = NewLoc.Z - InitialLocation.Z;

		// 到达目标高度
		if (CurrentHeight >= RaisedHeight)
		{
			ChangeState(EGateState::Raised);
		}
	}
	else if (CurrentState == EGateState::Lowering)
	{
		// 目标是回到初始位置
		FVector CurrentLoc = GateMesh->GetRelativeLocation();
		FVector TargetLoc = InitialLocation;

		// 匀速向下移动
		FVector NewLoc = FMath::VInterpConstantTo(CurrentLoc, TargetLoc, DeltaTime, LowerSpeed);
		GateMesh->SetRelativeLocation(NewLoc);

		// 更新当前高度
		CurrentHeight = NewLoc.Z - InitialLocation.Z;

		// 到达最低位置
		if (CurrentHeight <= 0.0f)
		{
			ChangeState(EGateState::Lowered);
		}
	}
}

void ARisingGate::ChangeState(EGateState NewState)
{
	CurrentState = NewState;

	if (CurrentState == EGateState::Raised)
	{
		// 闸门完全升起后，保持状态，直到玩家离开
	}
	else if (CurrentState == EGateState::Lowered)
	{
		// 闸门完全降下后
	}
}

void ARisingGate::CheckPlayerProximity()
{
	// 检查触发区域内是否有玩家
	TArray<AActor*> OverlappingActors;
	TriggerBox->GetOverlappingActors(OverlappingActors, ATank::StaticClass());

	bool bHasPlayer = false;
	for (AActor* Actor : OverlappingActors)
	{
		if (Cast<ATank>(Actor))
		{
			bHasPlayer = true;
			break;
		}
	}

	// 根据是否有玩家在区域内来控制闸门状态
	if (bHasPlayer)
	{
		if (CurrentState == EGateState::Lowered || CurrentState == EGateState::Lowering)
		{
			ChangeState(EGateState::Rising);
		}
	}
	else
	{
		if (CurrentState == EGateState::Raised || CurrentState == EGateState::Rising)
		{
			ChangeState(EGateState::Lowering);
		}
	}
}

void ARisingGate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 玩家进入触发区域，闸门上升
	ATank* EnteredTank = Cast<ATank>(OtherActor);
	if (EnteredTank)
	{
		if (CurrentState == EGateState::Lowered || CurrentState == EGateState::Lowering)
		{
			ChangeState(EGateState::Rising);
		}
	}
}

void ARisingGate::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 玩家离开触发区域，闸门下降
	ATank* LeftTank = Cast<ATank>(OtherActor);
	if (LeftTank)
	{
		if (CurrentState == EGateState::Raised || CurrentState == EGateState::Rising)
		{
			ChangeState(EGateState::Lowering);
		}
	}
}
