#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DestructibleProp.generated.h"

class UHealthComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UUserWidget;

UCLASS()
class BATTLEBLASTER_API ADestructibleProp : public AActor
{
	GENERATED_BODY()

public:
	ADestructibleProp();

#if WITH_EDITOR
	// 编辑器内预览血条 UI
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	// 更新血量条显示
	void UpdateHealthBar();

	// 蓝图可调用：设置是否显示血量条
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetHealthBarVisibility(bool bVisible);

	// 定时检测玩家距离并更新血量条可见性
	UFUNCTION()
	void CheckPlayerDistance();

protected:
	virtual void BeginPlay() override;

	// --- 修改点1：增加场景根组件声明 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultSceneRoot;

	// 模型组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PropMesh;

	// 血量组件（直接复用你的组件！）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComp;

	// 头顶血量条组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidgetComp;

	// 血量条 Widget 类（可以在蓝图中选择）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	// 是否显示血量条（蓝图可配置，默认开启）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	bool bShowHealthBar = true;

	// 玩家进入此距离范围内时才显示血量条（蓝图可调节）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float DetectionRadius = 1500.0f;

	// 检测间隔（秒），降低检测频率以优化性能
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	float DetectionInterval = 0.5f;

	// 上一帧的血量条可见性状态（用于检测变化）
	bool bWasHealthBarVisible = false;

	// 距离检测定时器句柄
	FTimerHandle DetectionTimerHandle;

	// 绑定的死亡回调函数
	UFUNCTION()
	virtual void OnPropDestroyed(class UHealthComponent* InHealthComp, class AController* InstigatedBy, AActor* DamageCauser);

	// 真正的破坏表现逻辑（子类去重写）
	virtual void HandleDestruction();

	// 控制销毁的定时器
	FTimerHandle DestroyTimerHandle;

	// 追踪最后对油桶造成伤害的玩家（用于爆炸时归属击杀）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	AActor* LastAttacker;

	// 追踪最后攻击时间（用于清理过期记录）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float LastAttackTime;

protected:
	// 重写 TakeDamage 以确保血量条更新
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
};