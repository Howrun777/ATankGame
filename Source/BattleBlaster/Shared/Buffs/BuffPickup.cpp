#include "Shared/Buffs/BuffPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Stage/TankStageGameMode.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Shared/Pawns/Tank.h"

ABuffPickup::ABuffPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(10.0f);
	SetMinNetUpdateFrequency(2.0f);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	RootComponent = SphereComp;
	SphereComp->SetSphereRadius(60.0f);
	SphereComp->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SphereComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetupAttachment(RootComponent);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

	IdleParticleComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("IdleParticleComp"));
	IdleParticleComp->SetupAttachment(RootComponent);

	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ABuffPickup::OnOverlapBegin);
}

void ABuffPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABuffPickup, CurrentVisualType);
	DOREPLIFETIME(ABuffPickup, bIsPickupAvailable);
}

void ABuffPickup::BeginPlay()
{
	Super::BeginPlay();

	BaseLocation = GetActorLocation();

	if (HasAuthority())
	{
		InitializeRandomBuff();
		bIsPickupAvailable = true;
	}

	ApplyBuffVisual();
	ApplyAvailabilityVisual();
}

void ABuffPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AddActorLocalRotation(FRotator(0.0f, RotateSpeed * DeltaTime, 0.0f));

	FVector NewLocation = BaseLocation;
	NewLocation.Z += FMath::Sin(GetWorld()->GetTimeSeconds() * HoverSpeed) * HoverAmplitude;
	SetActorLocation(NewLocation);
}

void ABuffPickup::InitializeRandomBuff()
{
	if (BuffVisualMap.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("ABuffPickup: BuffVisualMap is empty."));
		return;
	}

	TArray<EBuffType> AvailableBuffs;
	BuffVisualMap.GetKeys(AvailableBuffs);

	if (AvailableBuffs.Num() <= 0)
	{
		return;
	}

	CurrentVisualType = AvailableBuffs[FMath::RandRange(0, AvailableBuffs.Num() - 1)];
	ApplyBuffVisual();
}

void ABuffPickup::ApplyBuffVisual()
{
	if (!MeshComp)
	{
		return;
	}

	if (FBuffVisualData* FoundVisualData = BuffVisualMap.Find(CurrentVisualType))
	{
		if (FoundVisualData->BuffMesh)
		{
			MeshComp->SetStaticMesh(FoundVisualData->BuffMesh);
			MeshComp->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
		}

		if (FoundVisualData->BuffMaterial)
		{
			MeshComp->SetMaterial(0, FoundVisualData->BuffMaterial);
		}
	}
}

void ABuffPickup::ApplyAvailabilityVisual()
{
	if (MeshComp)
	{
		MeshComp->SetVisibility(bIsPickupAvailable, true);
	}

	if (IdleParticleComp)
	{
		IdleParticleComp->SetVisibility(bIsPickupAvailable, true);
		if (bIsPickupAvailable)
		{
			IdleParticleComp->Activate(true);
		}
		else
		{
			IdleParticleComp->Deactivate();
		}
	}

	if (SphereComp)
	{
		SphereComp->SetCollisionEnabled(bIsPickupAvailable && HasAuthority() ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}

void ABuffPickup::OnRep_BuffPickupState()
{
	ApplyBuffVisual();
	ApplyAvailabilityVisual();
}

void ABuffPickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bIsPickupAvailable)
	{
		return;
	}

	ATank* PlayerTank = Cast<ATank>(OtherActor);
	if (!PlayerTank || !PlayerTank->GetBuffComponent())
	{
		return;
	}

	EBuffType TypeToGive = CurrentVisualType;

	if (CurrentVisualType == EBuffType::RandomIcon)
	{
		TArray<EBuffType> RealBuffs;
		for (const TPair<EBuffType, FBuffVisualData>& Pair : BuffVisualMap)
		{
			if (Pair.Key != EBuffType::RandomIcon)
			{
				RealBuffs.Add(Pair.Key);
			}
		}

		if (RealBuffs.Num() > 0)
		{
			TypeToGive = RealBuffs[FMath::RandRange(0, RealBuffs.Num() - 1)];
		}
	}

	UTexture2D* IconToPass = nullptr;
	float Duration = 30.0f;

	if (FBuffVisualData* FoundData = BuffVisualMap.Find(TypeToGive))
	{
		IconToPass = FoundData->UIIcon;
		Duration = FoundData->BuffDuration;
	}

	PlayerTank->GetBuffComponent()->AddBuff(TypeToGive, Duration, IconToPass);
	MulticastPlayPickupEffects();
	HideAndStartRespawnTimer();
}

void ABuffPickup::MulticastPlayPickupEffects_Implementation()
{
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), PickupEffect, GetActorLocation());
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), PickupSound, GetActorLocation());
	}
}

void ABuffPickup::HideAndStartRespawnTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsPickupAvailable = false;
	ApplyAvailabilityVisual();

	AGameModeBase* CurrentGM = UGameplayStatics::GetGameMode(this);
	const bool bIsSinglePlayerMode = CurrentGM && CurrentGM->IsA(ATankStageGameMode::StaticClass());

	if (!bIsSinglePlayerMode)
	{
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABuffPickup::RespawnBuff, RespawnTime, false);
	}
}

void ABuffPickup::RespawnBuff()
{
	if (!HasAuthority())
	{
		return;
	}

	InitializeRandomBuff();
	bIsPickupAvailable = true;
	ApplyAvailabilityVisual();
}
