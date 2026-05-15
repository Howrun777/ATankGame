
#include "Shared/Pawns/BasePawn.h"
#include "Core/BattleBlasterCollisionChannels.h"
#include "Kismet/GameplayStatics.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/Combat/HealthComponent.h"

// Sets default values
ABasePawn::ABasePawn()
{
	PrimaryActorTick.bCanEverTick = true;
	// 1. 创建并设置胶囊体为根组件
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	SetRootComponent(CapsuleComp); // 设置为根
	CapsuleComp->InitCapsuleSize(55.f, 100.f);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);
	CapsuleComp->SetCollisionResponseToChannel(BB_COLLISION_PROJECTILE, ECR_Block);

	// 2. 创建底座网格，并附加到胶囊体上
	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(RootComponent); // 附加到根组件

	// 3. 创建炮塔网格
	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetupAttachment(CapsuleComp); 

	//4. 设置弹幕生成点,将其附加到炮塔网格
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT
	("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(TurretMesh);

	// 【新增】创建生命值组件
	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
}

/**
 * 基类Pawn的炮塔旋转核心函数
 * 功能：计算炮塔指向目标位置的旋转角度，并让炮塔网格体仅绕Z轴（水平方向）旋转朝向目标
 * 适用场景：坦克/炮台等需要炮塔水平瞄准目标的Pawn类
 * @param LookAtTarget 目标位置的世界空间坐标（如鼠标点击点、敌方位置）
 */
void ABasePawn::RotateTurret(FVector LookAtTarget)
{
	// 1. 计算从炮塔当前位置指向目标位置的方向向量
	// TurretMesh->GetComponentLocation()：获取炮塔网格体的世界空间中心位置
	// LookAtTarget - 炮塔位置：得到“炮塔→目标”的方向向量（仅表示方向和距离，无旋转信息）
	FVector VectorToTarget = LookAtTarget - TurretMesh->GetComponentLocation();

	// 2. 从方向向量提取旋转信息，并仅保留水平旋转（Yaw偏航）
	// VectorToTarget.Rotation()：将方向向量转换为对应的旋转器（包含Pitch/Yaw/Roll）
	// FRotator(0, Yaw, 0)：强制将俯仰（Pitch）和翻滚（Roll）设为0，避免炮塔上下/侧翻（符合炮塔仅水平旋转的设计）
	FRotator LookAtRotation = FRotator(0.0f, VectorToTarget.Rotation().Yaw, 0.0f);

	// 3. 计算平滑插值后的旋转（核心优化：避免炮塔瞬间转向，实现流畅的旋转过渡）
	// FMath::RInterpTo：UE内置的旋转插值函数，基于帧间隔实现线性插值旋转
	// 参数1：插值起始值 - 炮塔当前的实际旋转角度（从组件获取）
	// 参数2：插值目标值 - 计算出的朝向目标的理想旋转角度
	// 参数3：时间步长 - 从上一帧到当前帧的时间间隔（GetDeltaSeconds()保证旋转速度与帧率无关）
	// 参数4：插值速度 - 旋转的平滑系数（数值越大旋转越快，10.0f为兼顾手感的经验值，可根据需求调整）
	FRotator InterpolatedRotation = FMath::RInterpTo(
		TurretMesh->GetComponentRotation(),	// 起始旋转
		LookAtRotation,						// 目标旋转
		GetWorld()->GetDeltaSeconds(),		// 帧间隔时间
		10.0f								// 插值速度
	);

	// 4. 将平滑插值后的旋转应用到炮塔网格体
	// 替换直接设置目标旋转的方式，让炮塔以渐变方式转向目标，提升游戏视觉反馈
	TurretMesh->SetWorldRotation(InterpolatedRotation);
}

void ABasePawn::Fire()
{
	// 获取当前游戏时间（秒）
	float CurrentTime = GetWorld()->GetTimeSeconds();
	// 检查是否还在冷却期：当前时间 - 上次发射时间 < 冷却时间 → 不执行发射
	if (CurrentTime - Fire_LastTime < Fire_Interval) {
		return; // 冷却未结束，直接退出
	}
	FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
	FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

	////以下两个函数用于生成调试线条
	//DrawDebugSphere(GetWorld(), SpawnLocation, 25.0f, 12, FColor::Red, false, 5.0f);

	///*要计算 SpawnLocation 沿 SpawnRotation 方向延伸 1000.0f 的坐标，
	//核心思路是将旋转器转换为前向单位向量 → 缩放向量长度至 1000 → 与原始位置叠加*/
	//FVector TargetLocation = SpawnLocation + SpawnRotation.Vector() * 1000.0f;
	//DrawDebugLine(GetWorld(), SpawnLocation, TargetLocation, FColor::Blue,false,5.0f);

	AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(ProjectileClass,SpawnLocation,SpawnRotation);
	if (Projectile) {
		Projectile->SetOwner(this);
		// ========== 新增：更新上次发射时间 ==========
		Fire_LastTime = CurrentTime;
	}
}

void ABasePawn::HandleDestruction()
{
	if (DeathParticles) {
		// 在当前Pawn的位置和旋转角度，实例化并显示死亡粒子特效
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles,GetActorLocation(),GetActorRotation());
	}
	if (DeathSound) {
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeathSound, GetActorLocation());
	}
	// DeathCameraShakeClass通常是UCameraShake的子类，用于定义相机震动的幅度、频率等参数
	// 处理相机震动
	if (DeathCameraShakeClass)
	{
		// 不要获取 Player 0，而是获取"谁在控制这个Pawn"
		// GetController() 是 APawn 的成员函数，返回当前控制该 Pawn 的 Controller
		ATankPlayerController* PC = Cast<ATankPlayerController>(GetController());

		// 只有当控制者是玩家时才震动 (如果是 AI 控制的坦克，GetController会返回 AIController，这里转换就会失败，正好不震动，符合逻辑)
		if (PC)
		{
			PC->ClientStartCameraShake(DeathCameraShakeClass);
		}
	}
}



//
