#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RisingGate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

// ================= 闸门的运行状态 =================
UENUM(BlueprintType)
enum class EGateState : uint8
{
	Lowered     UMETA(DisplayName = "Lowered"),       // 闸门处于最低位置（关闭）
	Rising      UMETA(DisplayName = "Rising"),        // 正在上升（打开中）
	Raised      UMETA(DisplayName = "Raised"),        // 闸门完全升起（打开）
	Lowering    UMETA(DisplayName = "Lowering")       // 正在下降（关闭中）
};

UCLASS()
class BATTLEBLASTER_API ARisingGate : public AActor
{
	GENERATED_BODY()

public:
	ARisingGate();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// 改变闸门状态
	void ChangeState(EGateState NewState);

	// 检测玩家是否在触发区域内
	void CheckPlayerProximity();

	// 玩家进入触发区域
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 玩家离开触发区域
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	// ================= 组件 =================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootComp;

	// 闸门模型组件（随时间上下移动）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GateMesh;

	// 触发区域（用于检测玩家接近）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	// ================= 暴露给蓝图的参数 =================

	// 闸门完全升起后的高度（Z轴偏移）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Settings")
	float RaisedHeight = 200.0f;

	// 闸门上升速度（单位/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Settings")
	float RiseSpeed = 100.0f;

	// 闸门下降速度（单位/秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Settings")
	float LowerSpeed = 50.0f;

	// 玩家接近闸门时触发上升的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gate Settings")
	float TriggerDistance = 300.0f;

	// 当前闸门高度（只读，可在蓝图中使用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Settings")
	float CurrentHeight = 0.0f;

	// 当前状态（只读，可在蓝图中使用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gate Settings")
	EGateState CurrentState;

private:
	// 闸门初始位置
	FVector InitialLocation;

	// 记录是否有玩家在触发区域内
	bool bIsPlayerInTrigger;

	// 计时器句柄（用于延迟检测）
	FTimerHandle CheckTimerHandle;
};
