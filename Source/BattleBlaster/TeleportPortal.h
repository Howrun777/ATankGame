#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportPortal.generated.h"

class UBoxComponent;
class UArrowComponent;

UCLASS()
class BATTLEBLASTER_API ATeleportPortal : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATeleportPortal();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// 玩家进入触发区域
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 玩家完全离开触发区域
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 开启冷却
	void StartCooldown();

	// 重置冷却（定时器调用）
	UFUNCTION()
	void ResetCooldown();

public:
	// ================= 组件 =================

	// 根组件（用于挂载模型和特效）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootComp;

	// 触发区域（玩家碰到这个框就会传送）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBox;

	// 传送出口点（指示玩家传送后出现的位置和朝向）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* ExitPoint;

	// ================= 核心逻辑变量 =================

	// 只有 PortalPairID 相同的两个门才会互相传送
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Settings")
	int32 PortalPairID = 0;

	// 传送门冷却时间（默认2秒，可在蓝图中修改）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Portal Settings")
	float TeleportCooldown = 2.0f;

	// 当前是否处于冷却中
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal Status")
	bool bIsOnCooldown = false;

	// 缓存的另一扇门（配对目标）
	UPROPERTY(BlueprintReadOnly, Category = "Portal Status")
	ATeleportPortal* DestinationPortal = nullptr;

	// 豁免名单：在这个名单里的Actor，即使站在传送门上也不会被传送
	// （用于防止刚传过来就被瞬间传回去）
	UPROPERTY(BlueprintReadOnly, Category = "Portal Status")
	TArray<AActor*> IgnoredActors;

private:
	// 定时器句柄
	FTimerHandle CooldownTimerHandle;
};

/*
* 开发者须知:
* 1. 这个传送门放置的时候, 其坐标不是TriggerBox的坐标, 是其根组件的位置
* 2. 出生点ExitPoint的位置是相对根组件的,和TriggerBox的位置无关
* 3. TriggerBox必须包裹ExitPoint, 不然可能会无法触发豁免名单去除, 导致物体一直处于豁免名单, 无法再次触发传送门
* 4. 传送门触发后会进入冷却期(默认2秒), 冷却期间任何物体都无法触发传送
*/