#include "ExplosiveBarrel.h"
#include "GameFramework/Pawn.h"         // 用于转换攻击者为 Pawn（如玩家、坦克）
#include "NiagaraFunctionLibrary.h"    // 用于播放 Niagara 粒子特效（爆炸火光）
#include "Kismet/GameplayStatics.h"    // 用于播放音效、造成范围伤害等核心游戏功能
#include "Kismet/KismetSystemLibrary.h"
#include "Components/SphereComponent.h" // 用于可视化爆炸范围的球体组件
#include "HealthComponent.h"            // 你项目中的健康组件，用于受击对象扣血

// ------------------------------
// 构造函数：设置油桶的初始状态
// ------------------------------
AExplosiveBarrel::AExplosiveBarrel()
{
	// 1. 创建一个【球体组件】，专门用于在编辑器里“看见”爆炸范围
	// 就像在地图编辑器里放一个红色半透明球，告诉你炸到哪儿
	ExplosionRadiusVisualizer = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRadiusVisualizer"));

	// 2. 把这个球体“粘”在油桶的根组件上，油桶移动它也跟着动
	ExplosionRadiusVisualizer->SetupAttachment(RootComponent);

	// 3. 把球体的大小设置成你定义的爆炸半径变量（ExplosionRadius）
	ExplosionRadiusVisualizer->SetSphereRadius(ExplosionRadius);

	// 4. 设置这个球体的“性格”：
	//    - 无碰撞：不会挡着玩家走路或子弹
	//    - 可见：在编辑器里能看见
	//    - 游戏中默认显示（但 BeginPlay 会把它关掉）
	ExplosionRadiusVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExplosionRadiusVisualizer->SetVisibility(true);
	ExplosionRadiusVisualizer->bHiddenInGame = false;
}

// ------------------------------
// 编辑器专属函数：当你在编辑器里改参数时自动触发
// ------------------------------
#if WITH_EDITOR
void AExplosiveBarrel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 【核心逻辑】如果你在编辑器细节面板里修改了“ExplosionRadius”（爆炸半径）...
	if (PropertyChangedEvent.GetPropertyName() == TEXT("ExplosionRadius"))
	{
		// ...就自动把那个可视化球体的大小也改了，方便你实时预览范围
		if (ExplosionRadiusVisualizer)
		{
			ExplosionRadiusVisualizer->SetSphereRadius(ExplosionRadius);
		}
	}
}
#endif

// ------------------------------
// 游戏开始时调用（第1帧）
// ------------------------------
void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();

	// 游戏正式开始后，把那个红色的范围球藏起来，玩家不需要看见
	ExplosionRadiusVisualizer->SetVisibility(false);
	ExplosionRadiusVisualizer->bHiddenInGame = true;
}

// ------------------------------
// 每帧调用（暂时没逻辑，留着扩展）
// ------------------------------
void AExplosiveBarrel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// ------------------------------
// 【核心爆炸逻辑】当油桶被摧毁时执行这一切
// ------------------------------
void AExplosiveBarrel::HandleDestruction()
{
	// ==================================================
	// 步骤 0：精准定位油桶位置（非常重要！）
	// ==================================================
	// 【注意】必须在隐藏 Mesh 之前拿位置！
	// 如果先把模型藏了，有时候 GetActorLocation() 会抽到风（返回错误坐标）
	// 这里优先用 PropMesh（油桶的物理模型）的位置，更精准
	FVector RealBarrelLocation = PropMesh ? PropMesh->GetComponentLocation() : GetActorLocation();

	// ==================================================
	// 步骤 1：让油桶“消失”并“躺平”
	// ==================================================
	if (PropMesh)
	{
		PropMesh->SetVisibility(false);                      // 把油桶模型变透明（看不见了）
		PropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 关掉碰撞，玩家/子弹能穿过去
	}

	// ==================================================
	// 步骤 2：播放视觉和听觉盛宴（爆炸特效+音效）
	// ==================================================
	// 在刚才拿到的【真实位置】生成 Niagara 爆炸特效（火光、烟雾）
	if (ExplosionEffect) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, RealBarrelLocation);
	// 在同样位置播放爆炸音效
	if (ExplosionSound) UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, RealBarrelLocation);

	// ==================================================
	// 步骤 3：计算并造成【范围伤害】（核心中的核心）
	// ==================================================
	const float MaxDamage = ExplosionDamage; // 爆炸中心的最大伤害
	const float MinDamage = 0.0f;            // 爆炸边缘的最小伤害（刮擦伤害）

	// 【小技巧】把爆炸原点稍微往上抬 50 厘米（Z轴）
	// 因为油桶是放在地上的，抬一点更符合真实爆炸的感觉（不会只炸脚脖子）
	FVector ExplosionOrigin = RealBarrelLocation + FVector(0.0f, 0.0f, 50.0f);

	// 准备一个“忽略列表”：这些对象不会受到伤害
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this); // 必须忽略自己！不然油桶爆炸会把自己再炸一次，死循环

	// 找出是谁干的（攻击者），用于伤害归因（比如算击杀数）
	AActor* KillerActor = LastAttacker;
	AController* KillerController = nullptr;

	if (KillerActor)
	{
		// 【修复点】这里原来把 KillerActor 也加入了 IgnoreList，导致玩家炸桶不伤自己
		// 现在我们把这行删掉了！这样玩家如果离太近，也会被炸飞（真实感++）

		// 如果攻击者是一个 Pawn（比如玩家控制的坦克），把它的 Controller 拿过来
		if (APawn* KillerPawn = Cast<APawn>(KillerActor))
		{
			KillerController = KillerPawn->GetController();
		}
	}

	// 【调用 UE 原生神级函数】造成带衰减的范围伤害
	// 简单说就是：离得越近伤得越重，离得越远伤得越轻
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,                       // 伤害的“源头”是这个油桶
		MaxDamage,                  // 中心最大伤害
		MinDamage,                  // 边缘最小伤害
		ExplosionOrigin,            // 爆炸中心点（刚才往上抬了50的那个位置）
		0.0f,                       // 这个参数是“内部半径”，设为0就行
		ExplosionRadius,            // 爆炸最大半径
		DamageFalloff,              // 伤害衰减系数（控制伤害掉得有多快）
		UDamageType::StaticClass(), // 伤害类型（默认物理伤害）
		IgnoreActors,               // 忽略列表（只有油桶自己）
		this,                       // 伤害的 Instigator（还是油桶自己）
		KillerController,           // 攻击者的 Contrller（用于击杀统计）
		ECC_Visibility              // 检测阻挡的通道（用可见性通道，墙能挡住爆炸）
	);
	/*
	* ApplyRadialDamageWithFalloff 到底在背地里干了什么？
		你调用这行代码时，UE 引擎帮你完成了以下 3 件事：
		扫描（Trace）：它以 ExplosionOrigin 为中心，画了一个半径为 ExplosionRadius 的球。
		筛选：它找出这个球里所有开启了碰撞且能接受伤害的 Actor（比如你的坦克、敌人、其他油桶）。
		通知：它遍历这些 Actor，强制调用它们身上的 TakeDamage 函数。
	*/

	// ==================================================
	// 步骤 4：尘归尘，土归土（销毁油桶）
	// ==================================================
	Super::HandleDestruction(); // 建议先调用父类逻辑，放在最后防止父类提前清空数据
	Destroy();                   // 彻底把自己从地图上抹掉
}