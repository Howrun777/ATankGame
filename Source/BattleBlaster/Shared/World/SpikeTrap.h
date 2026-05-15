#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpikeTrap.generated.h"

class UBoxComponent;
class USphereComponent; // 前置声明
class UHealthComponent; // 【新增】前置声明生命值组件

// ================= 尖刺的运行状态 =================
UENUM(BlueprintType)
enum class ESpikeState : uint8
{
	Dormant     UMETA(DisplayName = "Dormant"),      // 休眠状态（玩家不在附近）
	Hidden      UMETA(DisplayName = "Hidden"),       // 潜伏在地下(正在计时)
	Thrusting   UMETA(DisplayName = "Thrusting"),    // 正在突然刺出
	Active      UMETA(DisplayName = "Active"),       // 保持在地面上
	Retracting  UMETA(DisplayName = "Retracting")    // 正在缩回地下
};

UCLASS()
class BATTLEBLASTER_API ASpikeTrap : public AActor
{
	GENERATED_BODY()

public:
	ASpikeTrap();
	virtual void Tick(float DeltaTime) override;
	// 增加事件与计数器：
	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


protected:
	virtual void BeginPlay() override;

	// 【修改】延迟一帧进行初始化检测
	void CheckInitialOverlaps();

	// 改变尖刺状态
	void ChangeState(ESpikeState NewState);
	// 定时器回调函数
	void OnHiddenTimerExpired();
	void OnActiveTimerExpired();

	// 【修改】函数改名：对所有合法目标造成伤害
	void DealDamageToActors();

	// 如果尖刺处于激活状态，有玩家不小心走上去，也会触发伤害
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	// 【新增】Actor 销毁或关卡结束时的安全清理逻辑
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	// ================= 组件 =================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootComp;

	// 尖刺模型组件（随时间上下移动）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SpikeMesh;

	// 伤害判定区域（挂在 SpikeMesh 上，随尖刺一起移动）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* DamageBox;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DetectionSphere;

	// ================= 暴露给蓝图的参数 =================
	// 在类的暴露参数区增加随机延迟设置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float RandomWakeDelayMax = 1.5f;      // 唤醒时的最大随机延迟（制造错乱感）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float DamageAmount = 40.0f;           // 伤害值

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float HiddenDuration = 5.0f;          // 缩入地下的潜伏时间（秒）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float ActiveDuration = 1.0f;          // 刺出后停留在地面的时间（秒）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float ZOffsetWhenHidden = -100.0f;    // 缩下时的 Z 轴深度（向下移动多少）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float ThrustSpeed = 800.0f;           // 突然刺出的速度（非常快）

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings")
	float RetractSpeed = 300.0f;          // 缩回地下的速度（相对慢一点）

	// 尖刺冒出时播放的音效
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spike Settings|Audio")
	class USoundBase* ThrustSound;
private:
	// 当前状态
	ESpikeState CurrentState;

	// 【新增】使用 TSet 替代数字计数，防止引擎重复触发 Overlap 导致永久无法休眠
	UPROPERTY()
	TSet<AActor*> ActorsInDetectionRange;

	// 【修改】不再只记录 Tank，而是记录所有受击 Actor，并换用 TSet 使得查找速度达到 O(1)
	UPROPERTY()
	TSet<AActor*> DamagedActorsThisCycle;

	// 状态计时器
	FTimerHandle StateTimerHandle;
};