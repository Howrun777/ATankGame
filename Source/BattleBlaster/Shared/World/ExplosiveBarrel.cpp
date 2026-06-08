#include "Shared/World/ExplosiveBarrel.h"
#include "Components/SphereComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

AExplosiveBarrel::AExplosiveBarrel()
{
	ExplosionRadiusVisualizer = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRadiusVisualizer"));
	ExplosionRadiusVisualizer->SetupAttachment(DefaultSceneRoot);
	ExplosionRadiusVisualizer->SetSphereRadius(ExplosionRadius);
	ExplosionRadiusVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExplosionRadiusVisualizer->SetVisibility(true);
	ExplosionRadiusVisualizer->bHiddenInGame = false;
}

#if WITH_EDITOR
void AExplosiveBarrel::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AExplosiveBarrel, ExplosionRadius))
	{
		if (ExplosionRadiusVisualizer)
		{
			ExplosionRadiusVisualizer->SetSphereRadius(ExplosionRadius);
		}
	}
}
#endif

void AExplosiveBarrel::BeginPlay()
{
	Super::BeginPlay();

	if (ExplosionRadiusVisualizer)
	{
		ExplosionRadiusVisualizer->SetVisibility(false);
		ExplosionRadiusVisualizer->bHiddenInGame = true;
	}
}

void AExplosiveBarrel::HandleDestruction()
{
	if (bIsDestroyed)
	{
		return;
	}

	const FVector RealBarrelLocation = GetDestructionEffectLocation();

	if (HasAuthority())
	{
		ApplyExplosionDamage(RealBarrelLocation);
		Super::HandleDestruction();
		SetLifeSpan(0.2f);
	}
	else
	{
		ApplyDestructionState();
	}
}

void AExplosiveBarrel::ApplyDestructionState()
{
	Super::ApplyDestructionState();

	if (PropMesh)
	{
		PropMesh->SetVisibility(false, true);
		PropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AExplosiveBarrel::PlayDestructionEffects(const FVector& EffectLocation)
{
	if (ExplosionEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect, EffectLocation);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, EffectLocation);
	}
}

FVector AExplosiveBarrel::GetDestructionEffectLocation() const
{
	return PropMesh ? PropMesh->GetComponentLocation() : GetActorLocation();
}

void AExplosiveBarrel::ApplyExplosionDamage(const FVector& RealBarrelLocation)
{
	const float MaxDamage = ExplosionDamage;
	const float MinDamage = 0.0f;
	const FVector ExplosionOrigin = RealBarrelLocation + FVector(0.0f, 0.0f, 50.0f);

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	AController* KillerController = nullptr;
	if (APawn* KillerPawn = Cast<APawn>(LastAttacker))
	{
		KillerController = KillerPawn->GetController();
	}

	UGameplayStatics::ApplyRadialDamageWithFalloff(
		this,
		MaxDamage,
		MinDamage,
		ExplosionOrigin,
		0.0f,
		ExplosionRadius,
		DamageFalloff,
		UDamageType::StaticClass(),
		IgnoreActors,
		this,
		KillerController,
		ECC_Visibility
	);
}
