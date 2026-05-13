#include "TeleportPortal.h"
#include "Components/BoxComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/ProjectileMovementComponent.h" 
#include "TimerManager.h" 
// 【已移除】 #include "Tank.h" 和 #include "Projectile.h" ，因为不再限制类型

ATeleportPortal::ATeleportPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComp);
	TriggerBox->SetBoxExtent(FVector(36.f, 150.f, 150.f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));

	ExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(RootComp);
	ExitPoint->SetRelativeLocation(FVector(30.f, 0.f, 59.0f));

	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ATeleportPortal::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &ATeleportPortal::OnOverlapEnd);
}

void ATeleportPortal::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> AllPortals;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATeleportPortal::StaticClass(), AllPortals);

	for (AActor* Actor : AllPortals)
	{
		ATeleportPortal* OtherPortal = Cast<ATeleportPortal>(Actor);
		if (OtherPortal && OtherPortal != this && OtherPortal->PortalPairID == this->PortalPairID)
		{
			DestinationPortal = OtherPortal;
			break;
		}
	}

	if (!DestinationPortal)
	{
		UE_LOG(LogTemp, Warning, TEXT("Portal (ID: %d) did not find a matching destination!"), PortalPairID);
	}
}

void ATeleportPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!DestinationPortal || !OtherActor) return;

	// ================= 1. 冷却检查 =================
	if (bIsOnCooldown)
	{
		return;
	}

	// 【已移除】 这里的 Tank 和 Projectile 强类型检查，现在任何碰到Trigger的Actor都可以往下走

	// ================= 2. 豁免名单检查 =================
	if (IgnoredActors.Contains(OtherActor))
	{
		return;
	}

	// 获取目标门的出口位置和朝向
	FVector DestLocation = DestinationPortal->ExitPoint->GetComponentLocation();
	FRotator DestRotation = DestinationPortal->ExitPoint->GetComponentRotation();

	// 加入目标门的豁免名单
	DestinationPortal->IgnoredActors.AddUnique(OtherActor);

	// ================= 3. 执行物理传送 =================
	OtherActor->SetActorLocationAndRotation(DestLocation, DestRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// ================= 4. 通用速度重定向 =================
	// 尝试获取该Actor是否拥有子弹移动组件
	UProjectileMovementComponent* ProjectileMovement = OtherActor->FindComponentByClass<UProjectileMovementComponent>();
	if (ProjectileMovement)
	{
		// 如果是子弹类物体，重定向其Velocity
		float CurrentSpeed = ProjectileMovement->Velocity.Size();
		ProjectileMovement->Velocity = DestRotation.Vector() * CurrentSpeed;
	}
	else
	{
		// 如果不是子弹，检查它是不是一个正在模拟物理的普通物体 (比如掉落的装备、滚动的箱子等)
		UPrimitiveComponent* RootPrimComp = Cast<UPrimitiveComponent>(OtherActor->GetRootComponent());
		if (RootPrimComp && RootPrimComp->IsSimulatingPhysics())
		{
			// 重定向普通物理组件的线速度，确保它们从门里飞出来的方向是正确的
			float CurrentSpeed = RootPrimComp->GetPhysicsLinearVelocity().Size();
			RootPrimComp->SetPhysicsLinearVelocity(DestRotation.Vector() * CurrentSpeed);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("%s successfully teleported to Portal Pair %d"), *OtherActor->GetName(), PortalPairID);

	// ================= 5. 触发冷却（两个门都要冷却） =================
	StartCooldown();
	DestinationPortal->StartCooldown();
}

void ATeleportPortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && IgnoredActors.Contains(OtherActor))
	{
		IgnoredActors.Remove(OtherActor);
	}
}

void ATeleportPortal::StartCooldown()
{
	bIsOnCooldown = true;

	GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &ATeleportPortal::ResetCooldown, TeleportCooldown, false);
}

void ATeleportPortal::ResetCooldown()
{
	bIsOnCooldown = false;

	GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("Portal %d cooldown finished. Ready to teleport again."), PortalPairID);
}