#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlideTrack.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class ATank;
// 在类声明的顶部加入前置声明
class USphereComponent;

UENUM(BlueprintType)
enum class ESlideTrackState : uint8
{
	State1_Speed UMETA(DisplayName = "Speed Up (State 1)"),
	State2_Slow UMETA(DisplayName = "Slow Down (State 2)")
};

UCLASS()
class BATTLEBLASTER_API ASlideTrack : public AActor
{
	GENERATED_BODY()

public:
	ASlideTrack();

protected:
	// ================= 性能与休眠控制 =================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* DetectionSphere;

	UFUNCTION()
	void OnDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY()
	TSet<ATank*> PlayersInDetectionRange;

	// 【新增】延迟一帧进行初始化检测
	void CheckInitialOverlaps();


	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ================= 组件部分 =================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* BoxComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* TrackParticleComp;

	// ================= 状态切换配置 =================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|Switch")
	bool bEnableStateSwitch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State1", meta = (EditCondition = "bEnableStateSwitch"))
	float State1Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State1")
	float State1SpeedMultiplier = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State1|Mesh")
	UStaticMesh* State1Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State1|Mesh")
	UMaterialInterface* State1Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State2", meta = (EditCondition = "bEnableStateSwitch"))
	float State2Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State2")
	float State2SpeedMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State2|Mesh")
	UStaticMesh* State2Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SlideTrack|State2|Mesh")
	UMaterialInterface* State2Material;

	// ================= 动画参数 =================
	UPROPERTY(EditDefaultsOnly, Category = "SlideTrack|Animation")
	float RotateSpeed = 45.0f;

	// ================= 状态变量 =================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlideTrack|State")
	ESlideTrackState CurrentState = ESlideTrackState::State1_Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlideTrack|State")
	float CurrentStateRemainingTime = 0.0f;

	UPROPERTY()
	TArray<ATank*> TanksOnTrack;

	// ================= 核心逻辑函数 =================
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnOverlapEnd(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	// 应用当前状态的速度效果给Tank
	void ApplySpeedEffect(ATank* Tank, float SpeedMultiplier);

	// 移除速度效果，恢复基础速度
	void RemoveSpeedEffect(ATank* Tank);

	// 切换到下一个状态
	void SwitchToNextState();

	// 更新网格体和材质
	void UpdateMeshAndMaterial();

	float GetCurrentStateDuration() const;
	float GetCurrentStateSpeedMultiplier() const;
};