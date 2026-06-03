#include "Modes/Network/NetworkGameModeBase.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkGameStateBase.h"
#include "Modes/Network/NetworkPlayerControllerBase.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/Pawns/Tank.h"
#include "TimerManager.h"

ANetworkGameModeBase::ANetworkGameModeBase()
{
	GameStateClass = ANetworkGameStateBase::StaticClass();
	PlayerStateClass = ANetworkPlayerStateBase::StaticClass();
	PlayerControllerClass = ANetworkPlayerControllerBase::StaticClass();
	DefaultPawnClass = nullptr;

	MaxNetworkPlayers = 4;
	bUseTaggedPlayerStarts = true;
	InitialAmmoRatio = 0.5f;
	RespawnDelay = 3.0f;
	RespawnHealthPercent = 1.0f;
	TankClass = ATank::StaticClass();
}

void ANetworkGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString MaxPlayersOption = UGameplayStatics::ParseOption(Options, TEXT("MaxPlayers"));
	if (!MaxPlayersOption.IsEmpty())
	{
		MaxNetworkPlayers = FMath::Clamp(FCString::Atoi(*MaxPlayersOption), 1, 8);
	}
}

void ANetworkGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	ActiveTanks.SetNum(MaxNetworkPlayers);

	if (ANetworkGameStateBase* NetworkGS = GetNetworkGameState())
	{
		NetworkGS->SetMaxNetworkPlayers(MaxNetworkPlayers);
		NetworkGS->SetConnectedPlayerCount(0);
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase BeginPlay. MaxNetworkPlayers=%d"), MaxNetworkPlayers);
}

void ANetworkGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Display, TEXT("Network PostLogin: %s"), *GetNameSafe(NewPlayer));
	RefreshConnectedPlayerCount();
}

void ANetworkGameModeBase::Logout(AController* Exiting)
{
	if (ANetworkPlayerStateBase* NetworkPS = Exiting ? Exiting->GetPlayerState<ANetworkPlayerStateBase>() : nullptr)
	{
		const int32 SlotId = NetworkPS->GetSlotId();
		if (ActiveTanks.IsValidIndex(SlotId) && ActiveTanks[SlotId])
		{
			ActiveTanks[SlotId]->Destroy();
			ActiveTanks[SlotId] = nullptr;
		}

		UE_LOG(LogTemp, Display, TEXT("Network Logout: SlotId=%d TeamId=%d"), SlotId, NetworkPS->GetTeamId());
	}

	Super::Logout(Exiting);
	RefreshConnectedPlayerCount();
}

void ANetworkGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}

	if (ANetworkPlayerStateBase* NetworkPS = NewPlayer->GetPlayerState<ANetworkPlayerStateBase>())
	{
		if (NetworkPS->GetSlotId() < 0)
		{
			const int32 SlotId = AllocateSlotId();
			if (SlotId < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("NetworkGameModeBase: no free SlotId for %s"), *GetNameSafe(NewPlayer));
				NewPlayer->ClientTravel(TEXT("/Game/Maps/MainMenuLevel_1"), TRAVEL_Absolute);
				return;
			}

			InitializePlayerIdentity(NewPlayer, SlotId);
		}
	}

	SpawnTankForPlayer(NewPlayer);
	RefreshConnectedPlayerCount();
}

int32 ANetworkGameModeBase::AllocateSlotId() const
{
	const AGameStateBase* GS = GameState;
	if (!GS)
	{
		return -1;
	}

	for (int32 Candidate = 0; Candidate < MaxNetworkPlayers; ++Candidate)
	{
		bool bUsed = false;
		for (APlayerState* PlayerState : GS->PlayerArray)
		{
			const ANetworkPlayerStateBase* NetworkPS = Cast<ANetworkPlayerStateBase>(PlayerState);
			if (NetworkPS && NetworkPS->GetSlotId() == Candidate)
			{
				bUsed = true;
				break;
			}
		}

		if (!bUsed)
		{
			return Candidate;
		}
	}

	return -1;
}

void ANetworkGameModeBase::InitializePlayerIdentity(APlayerController* PlayerController, int32 SlotId) const
{
	ANetworkPlayerStateBase* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!NetworkPS)
	{
		return;
	}

	NetworkPS->SetSlotId(SlotId);
	NetworkPS->SetTeamId(ChooseTeamIdForSlot(SlotId));
	NetworkPS->SetReady(false);

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: assigned SlotId=%d TeamId=%d to %s"),
		NetworkPS->GetSlotId(),
		NetworkPS->GetTeamId(),
		*GetNameSafe(PlayerController));
}

AActor* ANetworkGameModeBase::FindSpawnPointForSlot(int32 SlotId) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FName WantedTag(*FString::Printf(TEXT("P%d"), SlotId));

	if (bUseTaggedPlayerStarts)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* PlayerStart = *It;
			if (PlayerStart && (PlayerStart->PlayerStartTag == WantedTag || PlayerStart->ActorHasTag(WantedTag)))
			{
				return PlayerStart;
			}
		}
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);

	if (PlayerStarts.IsValidIndex(SlotId))
	{
		return PlayerStarts[SlotId];
	}

	return PlayerStarts.Num() > 0 ? PlayerStarts[0] : nullptr;
}

void ANetworkGameModeBase::SpawnTankForPlayer(APlayerController* PlayerController)
{
	ANetworkPlayerStateBase* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!PlayerController || !NetworkPS)
	{
		return;
	}

	const int32 SlotId = NetworkPS->GetSlotId();
	if (SlotId < 0)
	{
		return;
	}

	if (!ActiveTanks.IsValidIndex(SlotId))
	{
		ActiveTanks.SetNum(FMath::Max(MaxNetworkPlayers, SlotId + 1));
	}

	if (ActiveTanks[SlotId] && IsValid(ActiveTanks[SlotId]))
	{
		if (ActiveTanks[SlotId]->GetIsAlive())
		{
			PlayerController->Possess(ActiveTanks[SlotId]);
			InitializeSpawnedTank(ActiveTanks[SlotId], PlayerController, NetworkPS, false);
			return;
		}

		ActiveTanks[SlotId]->OnKilled.RemoveDynamic(this, &ANetworkGameModeBase::HandleTankKilled);
		ActiveTanks[SlotId]->Destroy();
		ActiveTanks[SlotId] = nullptr;
	}

	AActor* SpawnPoint = FindSpawnPointForSlot(SlotId);
	const FVector SpawnLocation = SpawnPoint ? SpawnPoint->GetActorLocation() : FVector::ZeroVector;
	const FRotator SpawnRotation = SpawnPoint ? SpawnPoint->GetActorRotation() : FRotator::ZeroRotator;

	TSubclassOf<ATank> ClassToSpawn = TankClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ATank::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PlayerController;
	SpawnParams.Instigator = PlayerController->GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATank* NewTank = GetWorld()->SpawnActor<ATank>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (!NewTank)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkGameModeBase: failed to spawn Tank for SlotId=%d"), SlotId);
		return;
	}

	NewTank->SetSlotId(SlotId);
	NewTank->SetTeamId(NetworkPS->GetTeamId());

	PlayerController->Possess(NewTank);
	InitializeSpawnedTank(NewTank, PlayerController, NetworkPS, true);
	ActiveTanks[SlotId] = NewTank;

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: spawned Tank %s for SlotId=%d TeamId=%d"),
		*GetNameSafe(NewTank),
		SlotId,
		NetworkPS->GetTeamId());
}

void ANetworkGameModeBase::InitializeSpawnedTank(ATank* NewTank, APlayerController* PlayerController, ANetworkPlayerStateBase* NetworkPS, bool bResetResources)
{
	if (!NewTank || !PlayerController || !NetworkPS)
	{
		return;
	}

	NewTank->SetOwner(PlayerController);
	NewTank->SetSlotId(NetworkPS->GetSlotId());
	NewTank->SetTeamId(NetworkPS->GetTeamId());
	NewTank->SetIsAlive(true);
	NewTank->SetActorHiddenInGame(false);
	NewTank->SetActorTickEnabled(true);
	NewTank->SetActorEnableCollision(true);
	NewTank->SetCanBeDamaged(true);

	if (bResetResources && NewTank->HealthComp)
	{
		NewTank->HealthComp->CurrentHealth = NewTank->HealthComp->MaxHealth * RespawnHealthPercent;
		NewTank->HealthComp->CurrentShield = 0.0f;
		NewTank->HealthComp->UpdateHUD();
	}

	if (bResetResources)
	{
		const int32 RespawnAmmo = CalculateInitialAmmoForTank(NewTank);

		NewTank->CurrentAmmo = RespawnAmmo;
		NewTank->SetAmmo(RespawnAmmo);
	}

	NewTank->SetPlayerEnabled(true);

	if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(PlayerController))
	{
		if (TankPC->IsLocalController())
		{
			TankPC->SetHUDAmmo(NewTank->CurrentAmmo, NewTank->MaxAmmo);
		}
		else
		{
			TankPC->ClientSetHUDAmmo(NewTank->CurrentAmmo, NewTank->MaxAmmo);
		}
	}

	NewTank->OnKilled.RemoveDynamic(this, &ANetworkGameModeBase::HandleTankKilled);
	NewTank->OnKilled.AddDynamic(this, &ANetworkGameModeBase::HandleTankKilled);
}

int32 ANetworkGameModeBase::CalculateInitialAmmoForTank(const ATank* Tank) const
{
	if (!Tank)
	{
		return 0;
	}

	const float ClampedRatio = FMath::Clamp(InitialAmmoRatio, 0.0f, 1.0f);
	return FMath::Clamp(FMath::FloorToInt(Tank->MaxAmmo * ClampedRatio), 0, Tank->MaxAmmo);
}

int32 ANetworkGameModeBase::ChooseTeamIdForSlot(int32 SlotId) const
{
	return SlotId;
}

bool ANetworkGameModeBase::ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const
{
	return PlayerState != nullptr;
}

void ANetworkGameModeBase::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	HandleNetworkTankKilled(DeadTank, KillerTank);
}

void ANetworkGameModeBase::HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	if (!DeadTank)
	{
		return;
	}

	APlayerController* DeadPlayerController = Cast<APlayerController>(DeadTank->GetController());
	ANetworkPlayerStateBase* NetworkPS = DeadPlayerController ? DeadPlayerController->GetPlayerState<ANetworkPlayerStateBase>() : DeadTank->GetPlayerState<ANetworkPlayerStateBase>();
	if (!DeadPlayerController && NetworkPS)
	{
		DeadPlayerController = Cast<APlayerController>(NetworkPS->GetOwner());
	}

	const int32 SlotId = NetworkPS ? NetworkPS->GetSlotId() : DeadTank->GetSlotId();
	if (ActiveTanks.IsValidIndex(SlotId) && ActiveTanks[SlotId] == DeadTank)
	{
		if (NetworkPS)
		{
			NetworkPS->SetAlive(false);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: Tank killed. Dead=%s Killer=%s SlotId=%d"),
		*GetNameSafe(DeadTank),
		*GetNameSafe(KillerTank),
		SlotId);

	if (ATankPlayerController* DeadTankPC = Cast<ATankPlayerController>(DeadPlayerController))
	{
		DeadTankPC->ShowDeathScreenForOwner(RespawnDelay);
	}

	if (ShouldRespawnPlayer(NetworkPS))
	{
		ScheduleRespawn(DeadPlayerController);
	}

	CheckNetworkGameOver();
}

void ANetworkGameModeBase::CheckNetworkGameOver()
{
}

void ANetworkGameModeBase::ScheduleRespawn(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	if (RespawnDelay <= 0.0f)
	{
		RespawnPlayer(PlayerController);
		return;
	}

	FTimerHandle RespawnTimerHandle;
	FTimerDelegate RespawnDelegate;
	TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);
	RespawnDelegate.BindWeakLambda(this, [this, WeakPlayerController]()
	{
		if (APlayerController* ValidPlayerController = WeakPlayerController.Get())
		{
			RespawnPlayer(ValidPlayerController);
		}
	});

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

void ANetworkGameModeBase::RespawnPlayer(APlayerController* PlayerController)
{
	ANetworkPlayerStateBase* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!PlayerController || !NetworkPS)
	{
		return;
	}

	if (!ShouldRespawnPlayer(NetworkPS))
	{
		return;
	}

	if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(PlayerController))
	{
		TankPC->HideDeathScreenForOwner();
	}

	const int32 SlotId = NetworkPS->GetSlotId();
	if (SlotId < 0)
	{
		return;
	}

	ATank* OldTank = nullptr;
	if (ActiveTanks.IsValidIndex(SlotId))
	{
		OldTank = ActiveTanks[SlotId];
		ActiveTanks[SlotId] = nullptr;
	}
	if (!OldTank)
	{
		OldTank = Cast<ATank>(PlayerController->GetPawn());
	}

	if (OldTank && IsValid(OldTank))
	{
		OldTank->OnKilled.RemoveDynamic(this, &ANetworkGameModeBase::HandleTankKilled);
		if (PlayerController->GetPawn() == OldTank)
		{
			PlayerController->UnPossess();
		}
		OldTank->Destroy();
	}

	SpawnTankForPlayer(PlayerController);
}

void ANetworkGameModeBase::RefreshConnectedPlayerCount() const
{
	if (ANetworkGameStateBase* NetworkGS = GetNetworkGameState())
	{
		const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 0;
		NetworkGS->SetConnectedPlayerCount(PlayerCount);
	}
}

ANetworkGameStateBase* ANetworkGameModeBase::GetNetworkGameState() const
{
	return GetGameState<ANetworkGameStateBase>();
}
