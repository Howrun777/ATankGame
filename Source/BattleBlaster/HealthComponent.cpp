#include "HealthComponent.h"
#include "GameFramework/Actor.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 生命值组件通常不需要Tick，关掉省性能
	CurrentHealth = MaxHealth;
	MaxShield = MaxHealth;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		// 绑定拥有者的受伤事件
		Owner->OnTakeAnyDamage.AddDynamic(this, &UHealthComponent::OnDamageTaken);
	}
}

void UHealthComponent::OnDamageTaken(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || CurrentHealth <= 0.0f) return;

	bool bWasAlive = (CurrentHealth > 0.0f);
	float ActualDamage = Damage;

	// ================== 【护盾抵挡逻辑】 ==================
	if (CurrentShield > 0.0f)
	{
		if (CurrentShield >= ActualDamage)
		{
			CurrentShield -= ActualDamage;
			ActualDamage = 0.0f;
		}
		else
		{
			ActualDamage -= CurrentShield;
			CurrentShield = 0.0f;
		}
	}

	// ================== 【扣血逻辑】 ==================
	if (ActualDamage > 0.0f)
	{
		CurrentHealth -= ActualDamage;
		CurrentHealth = FMath::Max(CurrentHealth, 0.0f);
	}

	// 1. 广播血量变化事件 (把施暴者 InstigatedBy 一并传出去，方便震动或UI显示受击方向)
	OnHealthChanged.Broadcast(this, CurrentHealth, -ActualDamage, DamageType, InstigatedBy, DamageCauser);

	// ================== 【死亡逻辑】 ==================
	if (bWasAlive && CurrentHealth <= 0.0f)
	{
		// 2. 广播死亡事件 (重点：把凶手 InstigatedBy 和 DamageCauser 传出去！)
		OnDeath.Broadcast(this, InstigatedBy, DamageCauser);
	}
}

void UHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || CurrentHealth <= 0.0f) return;

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);

	// 广播血量变化 (治疗的 Delta 是正数)
	OnHealthChanged.Broadcast(this, CurrentHealth, Amount, nullptr, nullptr, nullptr);
}

void UHealthComponent::AddShield(float Amount)
{
	if (Amount <= 0.0f || CurrentHealth <= 0.0f) return;

	CurrentShield = FMath::Clamp(CurrentShield + Amount, 0.0f, MaxShield);

	// 护盾变化也可以触发事件，或者你可以新建一个 OnShieldChanged 委托
	OnHealthChanged.Broadcast(this, CurrentHealth, 0.0f, nullptr, nullptr, nullptr);
}

void UHealthComponent::ResetHealth()
{
	CurrentHealth = MaxHealth;
	CurrentShield = 0.0f;
	OnHealthChanged.Broadcast(this, CurrentHealth, 0.0f, nullptr, nullptr, nullptr);
}

void UHealthComponent::UpdateHUD()
{
	// 广播当前血量，触发 UI 刷新（用于复活后强制刷新）
	OnHealthChanged.Broadcast(this, CurrentHealth, 0.0f, nullptr, nullptr, nullptr);
}