#include "DestructibleProp.h"
#include "HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h" // 新增：引入SceneComponent头文件
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ADestructibleProp::ADestructibleProp()
{
	PrimaryActorTick.bCanEverTick = false;  // 禁用Tick，使用定时器检测

	// 1. 创建空的场景根组件
	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot; // 设为根

	// 2. 创建模型组件（不再设为根，而是附着到根上）
	PropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMesh"));
	PropMesh->SetupAttachment(RootComponent); // <--- 关键修改

	// 默认开启碰撞，阻挡玩家和子弹
	PropMesh->SetCollisionProfileName(TEXT("BlockAll"));

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	// 创建头顶血量条组件
	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComp"));
	HealthBarWidgetComp->SetupAttachment(PropMesh);
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::World);
	HealthBarWidgetComp->SetDrawAtDesiredSize(true);
	HealthBarWidgetComp->SetVisibility(false);
}

#if WITH_EDITOR
void ADestructibleProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 仅编辑器内预览用：强制创建并显示血条
	if (HealthBarWidgetComp && HealthBarWidgetClass)
	{
		HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);
		HealthBarWidgetComp->SetVisibility(true);

		// 用当前数值填充进度条预览
		float PreviewPercent = 1.0f;
		if (HealthComp)
		{
			PreviewPercent = HealthComp->MaxHealth > 0.0f
				? HealthComp->CurrentHealth / HealthComp->MaxHealth
				: 1.0f;
		}
		PreviewPercent = FMath::Clamp(PreviewPercent, 0.0f, 1.0f);

		UUserWidget* PreviewWidget = HealthBarWidgetComp->GetUserWidgetObject();
		if (PreviewWidget)
		{
			UProgressBar* PB = Cast<UProgressBar>(PreviewWidget->GetWidgetFromName(TEXT("HealthBar")));
			if (!PB)
			{
				for (TFieldIterator<FObjectProperty> It(PreviewWidget->GetClass()); It; ++It)
				{
					if (UObject* Obj = It->GetObjectPropertyValue_InContainer(PreviewWidget))
					{
						PB = Cast<UProgressBar>(Obj);
						if (PB) break;
					}
				}
			}
			if (PB)
			{
				PB->SetPercent(PreviewPercent);
			}
		}
	}
}
#endif

void ADestructibleProp::BeginPlay()
{
	Super::BeginPlay();

	// 1. 把死亡事件绑定到我们的函数上
	if (HealthComp)
	{
		HealthComp->OnDeath.AddDynamic(this, &ADestructibleProp::OnPropDestroyed);
	}

	// 2. 设置血量条 Widget 类
	if (HealthBarWidgetClass)
	{
		HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);

		// 【关键修改 1】：移除强制显示代码，确保其遵循构造函数中的 SetVisibility(false)
		// 删除原来的 HealthBarWidgetComp->SetVisibility(bShowHealthBar);

		// 立即更新一次血量条数值
		UpdateHealthBar();
	}

	// 3. 【关键修改 2】：与组件的实际初始状态（隐藏）保持绝对一致
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(false);
	}
	bWasHealthBarVisible = false;

	// 4. 启动定时器定期检测玩家距离
	if (DetectionInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DetectionTimerHandle,
			this,
			&ADestructibleProp::CheckPlayerDistance,
			DetectionInterval,
			true
		);
	}

	// 5. 【关键修改 3】：在 BeginPlay 末尾立即手动检测一次！
	// 防止定时器（0.5秒后才执行）带来初次显示的延迟感。
	CheckPlayerDistance();
}


void ADestructibleProp::OnPropDestroyed(UHealthComponent* InHealthComp, AController* InstigatedBy, AActor* DamageCauser)
{
	HandleDestruction();
}

void ADestructibleProp::HandleDestruction()
{
	// 父类只负责关掉碰撞，防止死后还能挡路
	PropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 死亡时隐藏血量条
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(false);
	}
}

void ADestructibleProp::UpdateHealthBar()
{
	// 1. 如果组件为空或血量组件为空，直接返回
	if (!HealthBarWidgetComp || !HealthComp)
	{
		return;
	}

	// 2. 如果不显示血量条，也直接返回
	if (!bShowHealthBar)
	{
		return;
	}

	// 3. 获取当前的 Widget
	UUserWidget* HealthBarWidget = HealthBarWidgetComp->GetUserWidgetObject();
	if (!HealthBarWidget)
	{
		return;
	}

	// 4. 查找 ProgressBar 控件
	UProgressBar* ProgressBar = Cast<UProgressBar>(HealthBarWidget->GetWidgetFromName(TEXT("HealthBar")));

	// 备用查找：名称查找失败时，遍历所有属性
	if (!ProgressBar)
	{
		for (TFieldIterator<FObjectProperty> It(HealthBarWidget->GetClass()); It; ++It)
		{
			if (UObject* Obj = It->GetObjectPropertyValue_InContainer(HealthBarWidget))
			{
				if (UProgressBar* Bar = Cast<UProgressBar>(Obj))
				{
					ProgressBar = Bar;
					break;
				}
			}
		}
	}

	// 5. 更新血量
	if (ProgressBar)
	{
		float CurrentHealth = HealthComp->CurrentHealth;
		float MaxHealth = HealthComp->MaxHealth;
		float Percent = (MaxHealth > 0.0f) ? (CurrentHealth / MaxHealth) : 0.0f;
		Percent = FMath::Clamp(Percent, 0.0f, 1.0f);

		ProgressBar->SetPercent(Percent);
	}
}

void ADestructibleProp::SetHealthBarVisibility(bool bVisible)
{
	bShowHealthBar = bVisible;
	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(bVisible);
	}
}

float ADestructibleProp::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float DamageApplied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 追踪最后攻击油桶的玩家（用于爆炸时归属击杀）
	if (EventInstigator)
	{
		AActor* Attacker = EventInstigator->GetPawn();
		if (Attacker)
		{
			LastAttacker = Attacker;
			LastAttackTime = GetWorld()->GetTimeSeconds();
		}
	}

	// 更新血量条
	UpdateHealthBar();

	return DamageApplied;
}
void ADestructibleProp::CheckPlayerDistance()
{
	// 1. 如果蓝图设置根本不需要显示血量条，直接隐藏并返回
	if (!bShowHealthBar)
	{
		// 【优化】：同步更新状态缓存 bWasHealthBarVisible
		if (bWasHealthBarVisible)
		{
			if (HealthBarWidgetComp) HealthBarWidgetComp->SetVisibility(false);
			bWasHealthBarVisible = false;
		}
		return;
	}

	// 2. 查找最近的玩家
	bool bShouldBeVisible = false;
	if (UWorld* World = GetWorld())
	{
		const float DetectionRadiusSquared = DetectionRadius * DetectionRadius;
		const FVector PropLocation = GetActorLocation();

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* CandidatePC = It->Get();
			APawn* CandidatePawn = CandidatePC ? CandidatePC->GetPawn() : nullptr;

			if (CandidatePawn && FVector::DistSquared(PropLocation, CandidatePawn->GetActorLocation()) <= DetectionRadiusSquared)
			{
				bShouldBeVisible = true;
				break;
			}
		}
	}

	// 5. 【性能核心】：只有状态发生改变时，才调用底层UI的显示/隐藏（避免每0.5秒重复调用）
	if (bShouldBeVisible != bWasHealthBarVisible)
	{
		if (HealthBarWidgetComp)
		{
			HealthBarWidgetComp->SetVisibility(bShouldBeVisible);
		}
		bWasHealthBarVisible = bShouldBeVisible; // 更新缓存状态
	}
}
