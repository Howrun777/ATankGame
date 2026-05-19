#include "Shared/World/WoodenCrate.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

void AWoodenCrate::HandleDestruction()
{
	Super::HandleDestruction();

	if (HasAuthority())
	{
		SetLifeSpan(TimeToDisappear);
	}
}

void AWoodenCrate::ApplyDestructionState()
{
	Super::ApplyDestructionState();

	if (PropMesh)
	{
		PropMesh->SetVisibility(false, true);
	}
}

void AWoodenCrate::PlayDestructionEffects(const FVector& EffectLocation)
{
	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, EffectLocation);
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, EffectLocation);
	}
}
