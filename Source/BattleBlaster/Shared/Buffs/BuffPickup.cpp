#include "Shared/Buffs/BuffPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h" // 用于在指定位置生成Niagara特效
#include "Kismet/GameplayStatics.h"    // 用于播放音效
#include "Shared/Pawns/Tank.h"                       // 你的玩家坦克类头文件
#include "GameFramework/GameModeBase.h"
#include "Modes/Stage/TankStageGameMode.h"

// 注意：你需要稍后给Tank类添加一个BuffComponent来管理Buff状态

ABuffPickup::ABuffPickup()
{
	// 允许这个Actor每帧调用Tick()。如果不需要可以设为false以提升性能。
	PrimaryActorTick.bCanEverTick = true;

	// 1. 创建球形碰撞体（作为根组件）
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = SphereComp; // 设为根组件，所有其他组件都挂在它下面
	SphereComp->SetSphereRadius(60.0f); // 设置碰撞半径为60cm
	// 设置碰撞配置："OverlapAllDynamic" 表示它会与所有动态物体产生重叠事件（不阻挡，只检测）
	SphereComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	// 确保根组件缩放也是正常的
	SphereComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	// 2. 创建静态网格体组件
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent); // 附加到根组件
	// 关闭网格体的碰撞：我们只需要SphereComp来检测碰撞，网格体纯碎为了看
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	// 3. 创建Niagara待机特效组件
	IdleParticleComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleParticleComp"));
	IdleParticleComp->SetupAttachment(RootComponent); // 附加到根组件

	// 4. 绑定重叠事件：当SphereComp碰到东西时，调用我们的OnOverlapBegin函数
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ABuffPickup::OnOverlapBegin);
}

void ABuffPickup::BeginPlay()
{
	Super::BeginPlay(); // 必须先调用父类的BeginPlay()

	BaseLocation = GetActorLocation(); // 记录出生时的位置，用于后面的上下浮动计算
	InitializeRandomBuff(); // 游戏开始时，随机生成一个Buff外观
}

void ABuffPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime); // 必须先调用父类的Tick()

	// 动画逻辑 1：让Buff缓慢自转
	// AddActorLocalRotation：给Actor在本地空间添加旋转
	// FRotator(Pitch, Yaw, Roll)，我们只绕Yaw轴（竖直方向）旋转
	AddActorLocalRotation(FRotator(0.0f, RotateSpeed * DeltaTime, 0.0f));

	// 动画逻辑 2：让Buff上下浮动（使用正弦曲线实现平滑的往复运动）
	float TimeSeconds = GetWorld()->GetTimeSeconds(); // 获取游戏运行的当前时间
	FVector NewLocation = BaseLocation; // 从初始位置开始计算

	// FMath::Sin 返回 -1 到 1 之间的值
	// 乘以 HoverAmplitude 就得到了 -10cm 到 10cm 的偏移量
	NewLocation.Z += FMath::Sin(TimeSeconds * HoverSpeed) * HoverAmplitude;

	// 设置新位置
	SetActorLocation(NewLocation);
}

void ABuffPickup::InitializeRandomBuff()
{
	// 【新增防呆设计】：确保蓝图中已经配置了Buff
	if (BuffVisualMap.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ABuffPickup: BuffVisualMap 是空的！请在 BP_BuffPickup 蓝图中配置它！"));
		return;
	}
	// 1. 获取Map中所有配置好的 BuffType 键值
	TArray<EBuffType> AvailableBuffs;
	BuffVisualMap.GetKeys(AvailableBuffs);
	// 2. 根据容器的实际长度进行随机抽取，彻底解耦数量限制！
	int32 RandomIndex = FMath::RandRange(0, AvailableBuffs.Num() - 1);
	CurrentVisualType = AvailableBuffs[RandomIndex];

	// 3. 应用视觉效果
	if (FBuffVisualData* FoundVisualData = BuffVisualMap.Find(CurrentVisualType))
	{
		if (FoundVisualData->BuffMesh)
		{
			MeshComp->SetStaticMesh(FoundVisualData->BuffMesh);
			MeshComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}

		if (FoundVisualData->BuffMaterial)
		{
			MeshComp->SetMaterial(0, FoundVisualData->BuffMaterial);
		}
	}
}

void ABuffPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ATank* PlayerTank = Cast<ATank>(OtherActor);
	if (PlayerTank && PlayerTank->GetBuffComponent())
	{
		EBuffType TypeToGive = CurrentVisualType;

		// 【解耦修改】：如果是问号，从配置好的真实Buff中随机抽取
		if (CurrentVisualType == EBuffType::RandomIcon)
		{
			TArray<EBuffType> RealBuffs;

			// 遍历 Map，把除了 RandomIcon 之外的所有 Buff 放进备选池
			for (const TPair<EBuffType, FBuffVisualData>& Pair : BuffVisualMap)
			{
				if (Pair.Key != EBuffType::RandomIcon)
				{
					RealBuffs.Add(Pair.Key);
				}
			}

			// 从真实的 Buff 池中随机抽取
			if (RealBuffs.Num() > 0)
			{
				int32 RandomIndex = FMath::RandRange(0, RealBuffs.Num() - 1);
				TypeToGive = RealBuffs[RandomIndex];
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("警告：BuffMap 中只配置了 RandomIcon，没有配置其他真实Buff！"));
			}
		}

		UTexture2D* IconToPass = nullptr;
		float Duration = 30.0f;

		// 统一安全读取给定的 Buff 数据（图标和持续时间）
		if (FBuffVisualData* FoundData = BuffVisualMap.Find(TypeToGive))
		{
			IconToPass = FoundData->UIIcon;
			Duration = FoundData->BuffDuration;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("警告：BP_BuffPickup 蓝图中没有配置类型为 %d 的贴图/时长数据！"), (int32)TypeToGive);
		}

		// 发送给玩家
		PlayerTank->GetBuffComponent()->AddBuff(TypeToGive, Duration, IconToPass);

		// 播放特效和音效
		if (PickupEffect)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PickupEffect, GetActorLocation());
		}
		if (PickupSound)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
		}

		HideAndStartRespawnTimer();
	}
}

void ABuffPickup::HideAndStartRespawnTimer()
{
	// 1. 隐藏实体（视觉上消失）
	MeshComp->SetVisibility(false);
	// 同时关闭碰撞，防止玩家在看不见的情况下还能“捡空气”
	SphereComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. 设置计时器
	// 单人闯关模式下，禁止 Buff 刷新
	AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(this);
	bool bIsSinglePlayerMode = CurrentGM && CurrentGM->IsA(ATankStageGameMode::StaticClass());

	if (!bIsSinglePlayerMode)
	{
		// 参数：计时器句柄、回调对象、回调函数、等待时间、是否循环、首次延迟（这里不循环，只延迟RespawnTime秒执行一次）
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABuffPickup::RespawnBuff, RespawnTime, false);
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("BuffPickup: TankStageGameMode - Respawn disabled for this pickup."));
	}
}

void ABuffPickup::RespawnBuff()
{
	// 1. 重新随机一个新的Buff外观
	InitializeRandomBuff();

	// 2. 恢复显示
	MeshComp->SetVisibility(true);
	// 恢复碰撞（设为QueryOnly，因为我们只需要查询重叠，不需要物理阻挡）
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}