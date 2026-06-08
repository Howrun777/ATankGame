#include "Shared/World/DestructibleProp.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Shared/Combat/HealthComponent.h"
#include "UObject/FieldIterator.h"
#include "UObject/UnrealType.h"

ADestructibleProp::ADestructibleProp()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);
	SetMinNetUpdateFrequency(5.0f);

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	PropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMesh"));
	PropMesh->SetupAttachment(RootComponent);
	PropMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PropMesh->SetIsReplicated(true);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	HealthBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComp"));
	HealthBarWidgetComp->SetupAttachment(PropMesh);
	HealthBarWidgetComp->SetWidgetSpace(EWidgetSpace::World);
	HealthBarWidgetComp->SetDrawAtDesiredSize(true);
	HealthBarWidgetComp->SetVisibility(false);
}

void ADestructibleProp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADestructibleProp, bIsDestroyed);
	DOREPLIFETIME(ADestructibleProp, ReplicatedPropMeshTransform);
}

#if WITH_EDITOR
void ADestructibleProp::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!HealthBarWidgetComp || !HealthBarWidgetClass)
	{
		return;
	}

	HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);
	HealthBarWidgetComp->SetVisibility(true);

	float PreviewPercent = 1.0f;
	if (HealthComp)
	{
		PreviewPercent = HealthComp->MaxHealth > 0.0f
			? HealthComp->CurrentHealth / HealthComp->MaxHealth
			: 1.0f;
	}
	PreviewPercent = FMath::Clamp(PreviewPercent, 0.0f, 1.0f);

	if (UUserWidget* PreviewWidget = HealthBarWidgetComp->GetUserWidgetObject())
	{
		UProgressBar* ProgressBar = Cast<UProgressBar>(PreviewWidget->GetWidgetFromName(TEXT("HealthBar")));
		if (!ProgressBar)
		{
			for (TFieldIterator<FObjectProperty> It(PreviewWidget->GetClass()); It; ++It)
			{
				if (UObject* Obj = It->GetObjectPropertyValue_InContainer(PreviewWidget))
				{
					ProgressBar = Cast<UProgressBar>(Obj);
					if (ProgressBar)
					{
						break;
					}
				}
			}
		}

		if (ProgressBar)
		{
			ProgressBar->SetPercent(PreviewPercent);
		}
	}
}
#endif

void ADestructibleProp::BeginPlay()
{
	Super::BeginPlay();

	ConfigurePropMeshForNetwork();

	if (HealthComp)
	{
		HealthComp->OnDeath.AddDynamic(this, &ADestructibleProp::OnPropDestroyed);
		HealthComp->OnHealthChanged.AddDynamic(this, &ADestructibleProp::OnPropHealthChanged);
	}

	if (HealthBarWidgetClass && HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetWidgetClass(HealthBarWidgetClass);
		UpdateHealthBar();
	}

	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(false);
	}
	bWasHealthBarVisible = false;

	if (DetectionInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DetectionTimerHandle,
			this,
			&ADestructibleProp::CheckPlayerDistance,
			DetectionInterval,
			true
		);
	}

	CheckPlayerDistance();

	if (HasAuthority() && ShouldReplicatePropMeshTransform())
	{
		UpdateReplicatedPropMeshTransform();
		GetWorldTimerManager().SetTimer(
			PropMeshTransformReplicationTimerHandle,
			this,
			&ADestructibleProp::UpdateReplicatedPropMeshTransform,
			PropMeshTransformReplicationInterval,
			true
		);
	}
}

void ADestructibleProp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SmoothReplicatedPropMeshTransform(DeltaTime);
}

void ADestructibleProp::OnPropDestroyed(UHealthComponent* InHealthComp, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!HasAuthority())
	{
		return;
	}

	HandleDestruction();
}

void ADestructibleProp::OnPropHealthChanged(UHealthComponent* InHealthComp, float Health, float HealthDelta, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	UpdateHealthBar();
}

void ADestructibleProp::OnRep_DestroyedState()
{
	if (bIsDestroyed)
	{
		ApplyDestructionState();
	}
}

void ADestructibleProp::OnRep_ReplicatedPropMeshTransform()
{
	if (!HasAuthority() && PropMesh && !bIsDestroyed)
	{
		PropMeshSmoothStartTransform = PropMesh->GetComponentTransform();
		PropMeshSmoothTargetTransform = ReplicatedPropMeshTransform;
		PropMeshSmoothElapsedTime = 0.0f;
		SetActorTickEnabled(true);
	}
}

void ADestructibleProp::MulticastHandleDestruction_Implementation(FVector EffectLocation)
{
	bIsDestroyed = true;
	ApplyDestructionState();
	PlayDestructionEffects(EffectLocation);
}

void ADestructibleProp::HandleDestruction()
{
	if (bIsDestroyed)
	{
		return;
	}

	if (!HasAuthority())
	{
		ApplyDestructionState();
		return;
	}

	bIsDestroyed = true;
	MulticastHandleDestruction(GetDestructionEffectLocation());
}

void ADestructibleProp::ApplyDestructionState()
{
	GetWorldTimerManager().ClearTimer(PropMeshTransformReplicationTimerHandle);

	if (PropMesh)
	{
		PropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(false);
	}

	bWasHealthBarVisible = false;
	GetWorldTimerManager().ClearTimer(DetectionTimerHandle);
}

void ADestructibleProp::PlayDestructionEffects(const FVector& EffectLocation)
{
}

FVector ADestructibleProp::GetDestructionEffectLocation() const
{
	return GetActorLocation();
}

void ADestructibleProp::ConfigurePropMeshForNetwork()
{
	if (!PropMesh)
	{
		return;
	}

	PropMesh->SetIsReplicated(true);

	if (!HasAuthority() && PropMesh->IsSimulatingPhysics())
	{
		PropMesh->SetSimulatePhysics(false);
	}
}

bool ADestructibleProp::ShouldReplicatePropMeshTransform() const
{
	return bReplicatePropMeshTransform && PropMesh && PropMesh->IsSimulatingPhysics();
}

void ADestructibleProp::UpdateReplicatedPropMeshTransform()
{
	if (HasAuthority() && ShouldReplicatePropMeshTransform())
	{
		const FTransform CurrentTransform = PropMesh->GetComponentTransform();
		const float LocationDeltaSquared = FVector::DistSquared(
			CurrentTransform.GetLocation(),
			ReplicatedPropMeshTransform.GetLocation());
		const float RotationDeltaDegrees = CurrentTransform.GetRotation().AngularDistance(
			ReplicatedPropMeshTransform.GetRotation()) * 180.0f / PI;
		const float LocationThresholdSquared = PropMeshTransformLocationThreshold * PropMeshTransformLocationThreshold;

		if (LocationDeltaSquared >= LocationThresholdSquared || RotationDeltaDegrees >= PropMeshTransformRotationThresholdDegrees)
		{
			ReplicatedPropMeshTransform = CurrentTransform;
			ForceNetUpdate();
		}
	}
}

void ADestructibleProp::SmoothReplicatedPropMeshTransform(float DeltaTime)
{
	if (HasAuthority() || !PropMesh || bIsDestroyed)
	{
		return;
	}

	if (PropMeshSmoothDuration <= KINDA_SMALL_NUMBER)
	{
		PropMesh->SetWorldTransform(PropMeshSmoothTargetTransform, false, nullptr, ETeleportType::TeleportPhysics);
		SetActorTickEnabled(false);
		return;
	}

	PropMeshSmoothElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(PropMeshSmoothElapsedTime / PropMeshSmoothDuration, 0.0f, 1.0f);
	const FVector NewLocation = FMath::Lerp(PropMeshSmoothStartTransform.GetLocation(), PropMeshSmoothTargetTransform.GetLocation(), Alpha);
	const FQuat NewRotation = FQuat::Slerp(PropMeshSmoothStartTransform.GetRotation(), PropMeshSmoothTargetTransform.GetRotation(), Alpha);
	const FVector NewScale = FMath::Lerp(PropMeshSmoothStartTransform.GetScale3D(), PropMeshSmoothTargetTransform.GetScale3D(), Alpha);

	PropMesh->SetWorldTransform(FTransform(NewRotation, NewLocation, NewScale), false, nullptr, ETeleportType::TeleportPhysics);

	if (Alpha >= 1.0f)
	{
		SetActorTickEnabled(false);
	}
}

void ADestructibleProp::UpdateHealthBar()
{
	if (!HealthBarWidgetComp || !HealthComp || !bShowHealthBar)
	{
		return;
	}

	UUserWidget* HealthBarWidget = HealthBarWidgetComp->GetUserWidgetObject();
	if (!HealthBarWidget)
	{
		return;
	}

	UProgressBar* ProgressBar = Cast<UProgressBar>(HealthBarWidget->GetWidgetFromName(TEXT("HealthBar")));
	if (!ProgressBar)
	{
		for (TFieldIterator<FObjectProperty> It(HealthBarWidget->GetClass()); It; ++It)
		{
			if (UObject* Obj = It->GetObjectPropertyValue_InContainer(HealthBarWidget))
			{
				ProgressBar = Cast<UProgressBar>(Obj);
				if (ProgressBar)
				{
					break;
				}
			}
		}
	}

	if (ProgressBar)
	{
		const float Percent = HealthComp->MaxHealth > 0.0f
			? FMath::Clamp(HealthComp->CurrentHealth / HealthComp->MaxHealth, 0.0f, 1.0f)
			: 0.0f;
		ProgressBar->SetPercent(Percent);
	}
}

void ADestructibleProp::SetHealthBarVisibility(bool bVisible)
{
	bShowHealthBar = bVisible;

	if (HealthBarWidgetComp)
	{
		HealthBarWidgetComp->SetVisibility(bVisible);
	}
}

float ADestructibleProp::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDestroyed)
	{
		return 0.0f;
	}

	const float DamageApplied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() && EventInstigator)
	{
		if (AActor* Attacker = EventInstigator->GetPawn())
		{
			LastAttacker = Attacker;
			LastAttackTime = GetWorld()->GetTimeSeconds();
		}
	}

	UpdateHealthBar();

	return DamageApplied;
}

void ADestructibleProp::CheckPlayerDistance()
{
	if (bIsDestroyed || !bShowHealthBar)
	{
		if (bWasHealthBarVisible)
		{
			if (HealthBarWidgetComp)
			{
				HealthBarWidgetComp->SetVisibility(false);
			}
			bWasHealthBarVisible = false;
		}
		return;
	}

	bool bShouldBeVisible = false;
	if (UWorld* World = GetWorld())
	{
		const float DetectionRadiusSquared = DetectionRadius * DetectionRadius;
		const FVector PropLocation = GetActorLocation();

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* CandidatePC = It->Get();
			APawn* CandidatePawn = CandidatePC ? CandidatePC->GetPawn() : nullptr;

			if (CandidatePawn && FVector::DistSquared(PropLocation, CandidatePawn->GetActorLocation()) <= DetectionRadiusSquared)
			{
				bShouldBeVisible = true;
				break;
			}
		}
	}

	if (bShouldBeVisible != bWasHealthBarVisible)
	{
		if (HealthBarWidgetComp)
		{
			HealthBarWidgetComp->SetVisibility(bShouldBeVisible);
		}
		bWasHealthBarVisible = bShouldBeVisible;
	}
}
