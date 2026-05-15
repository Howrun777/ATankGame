#pragma once
#include "CoreMinimal.h"
#include "Shared/Pawns/BasePawn.h"
#include "Shared/Pawns/Tank.h" 

#include "Tower.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USphereComponent; // 【新增】前置声明球体组件

UCLASS()
class BATTLEBLASTER_API ATower : public ABasePawn
{
    GENERATED_BODY()

public:
    ATower();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Fire() override;
    virtual void HandleDestruction() override;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRange = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float FireRate = 2.0f;

    // 【修改】获取目标逻辑保留，但内部实现会改变
    ATank* GetTargetInRange();

    bool IsTargetBlocked(ATank* Target);

    UFUNCTION(BlueprintCallable, Category = "Difficulty")
    void ApplyDifficultyMultiplier(float Multiplier);

    float GetCurrentDifficultyMultiplier() const { return CurrentDifficultyMultiplier; }

protected:
    // ==========================================
    // 【新增】侦测雷达（警戒网）相关
    // ==========================================

    // 球体碰撞组件，代表塔的探测范围
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* DetectionSphere;

    // 当有物体进入警戒网时触发
    UFUNCTION()
    void OnDetectionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    // 当有物体离开警戒网时触发
    UFUNCTION()
    void OnDetectionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // HealthComponent 事件处理（绑定 OnHealthChanged 和 OnDeath）
    UFUNCTION()
    void HandleTowerHealthChanged(UHealthComponent* InHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

    UFUNCTION()
    void HandleTowerDeath(UHealthComponent* InHealthComp, class AController* InstigatedBy, AActor* DamageCauser);

    // 记录当前在警戒网内的所有坦克（方便应对以后有多名玩家或盟友的情况）
    UPROPERTY()
    TArray<ATank*> TargetsInRange;

    // ... 下方的复活和特效变量保持不变 ...
    UPROPERTY(EditAnywhere, Category = "Respawn")
    UNiagaraSystem* DeathLoopEffect;
    UPROPERTY(EditAnywhere, Category = "Respawn")
    UNiagaraSystem* RespawnSuccessEffect;
    UPROPERTY(EditAnywhere, Category = "Respawn")
    float RespawnTotalTime = 60.0f;
    UPROPERTY(EditAnywhere, Category = "Respawn")
    float RespawnEffectDuration = 3.0f;

private:
    bool bIsDead = false;
    UPROPERTY()
    UNiagaraComponent* ActiveDeathLoopComponent;
    UPROPERTY()
    UNiagaraComponent* ActiveRespawnComponent;

    FTimerHandle TimerHandle_RespawnRevive;
    FTimerHandle TimerHandle_StopRespawnFx;

    void ReviveTower();
    void StopRespawnEffect();
    void SetTowerState(bool bActive);

private:
    float CurrentDifficultyMultiplier = 1.0f;
};