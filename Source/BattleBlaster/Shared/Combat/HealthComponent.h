#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

// 1. 定义委托：当血量改变时广播（用于UI更新、伤害数字、受击震动等）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnHealthChangedSignature, UHealthComponent*, HealthComp, float, Health, float, HealthDelta, const class UDamageType*, DamageType, class AController*, InstigatedBy, AActor*, DamageCauser);

// 2. 定义委托：当死亡时广播（把凶手传出去，Tank和GameMode靠这个获取击杀者）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDeathSignature, UHealthComponent*, HealthComp, class AController*, InstigatedBy, AActor*, DamageCauser);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BATTLEBLASTER_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

protected:
	virtual void BeginPlay() override;

	// 组件内部依然监听拥有者的伤害事件
	UFUNCTION()
	void OnDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:
	// 对外暴露的属性，不要写死任何 Tank 相关的逻辑
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 200.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxShield = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	float CurrentShield;

	// 对外暴露的委托（事件），其他类可以通过绑这两个事件来执行逻辑
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeathSignature OnDeath;

	// 通用功能函数
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void AddShield(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	// 辅助获取百分比
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetShieldPercent() const { return MaxShield > 0.0f ? CurrentShield / MaxShield : 0.0f; }

	// 刷新 HUD（由 Tank/GameMode 在复活时调用，通知 UI 同步显示当前血量）
	UFUNCTION(BlueprintCallable, Category = "Health")
	void UpdateHUD();
};