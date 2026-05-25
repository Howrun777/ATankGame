#include "Modes/Network/NetworkBattleGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkBattleGameState.h"
#include "Modes/Network/NetworkBattlePlayerController.h"
#include "Modes/Network/NetworkBattlePlayerState.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/Pawns/Tank.h"
#include "TimerManager.h"

ANetworkBattleGameMode::ANetworkBattleGameMode()
{
	GameStateClass = ANetworkBattleGameState::StaticClass();
	PlayerStateClass = ANetworkBattlePlayerState::StaticClass();
	PlayerControllerClass = ANetworkBattlePlayerController::StaticClass();
	DefaultPawnClass = nullptr;

	MaxNetworkPlayers = 4;
	bUseTaggedPlayerStarts = true;
	InitialAmmoRatio = 0.5f;
	RespawnDelay = 3.0f;
	RespawnHealthPercent = 1.0f;
	TankClass = ATank::StaticClass();
}

void ANetworkBattleGameMode::BeginPlay()
{
	Super::BeginPlay();

	ActiveTanks.SetNum(MaxNetworkPlayers);

	if (ANetworkBattleGameState* NetworkGS = GetNetworkGameState())
	{
		NetworkGS->SetMaxNetworkPlayers(MaxNetworkPlayers);
		NetworkGS->SetConnectedPlayerCount(0);
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkBattleGameMode BeginPlay. MaxNetworkPlayers=%d"), MaxNetworkPlayers);
}

void ANetworkBattleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	UE_LOG(LogTemp, Display, TEXT("Network PostLogin: %s"), *GetNameSafe(NewPlayer));
	RefreshConnectedPlayerCount();
}

void ANetworkBattleGameMode::Logout(AController* Exiting)
{
	if (ANetworkBattlePlayerState* NetworkPS = Exiting ? Exiting->GetPlayerState<ANetworkBattlePlayerState>() : nullptr)
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

void ANetworkBattleGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return;
	}

	if (ANetworkBattlePlayerState* NetworkPS = NewPlayer->GetPlayerState<ANetworkBattlePlayerState>())
	{
		if (NetworkPS->GetSlotId() < 0)
		{
			const int32 SlotId = AllocateSlotId();
			if (SlotId < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("NetworkBattle: no free SlotId for %s"), *GetNameSafe(NewPlayer));
				NewPlayer->ClientTravel(TEXT("/Game/Maps/MainMenuLevel_1"), TRAVEL_Absolute);
				return;
			}

			InitializePlayerIdentity(NewPlayer, SlotId);
		}
	}

	SpawnTankForPlayer(NewPlayer);
	RefreshConnectedPlayerCount();
}

int32 ANetworkBattleGameMode::AllocateSlotId() const
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
			const ANetworkBattlePlayerState* NetworkPS = Cast<ANetworkBattlePlayerState>(PlayerState);
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

void ANetworkBattleGameMode::InitializePlayerIdentity(APlayerController* PlayerController, int32 SlotId) const
{
	ANetworkBattlePlayerState* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkBattlePlayerState>() : nullptr;
	if (!NetworkPS)
	{
		return;
	}

	NetworkPS->SetSlotId(SlotId);
	NetworkPS->SetTeamId(SlotId);
	NetworkPS->SetReady(false);

	UE_LOG(LogTemp, Display, TEXT("NetworkBattle: assigned SlotId=%d TeamId=%d to %s"),
		NetworkPS->GetSlotId(),
		NetworkPS->GetTeamId(),
		*GetNameSafe(PlayerController));
}

AActor* ANetworkBattleGameMode::FindSpawnPointForSlot(int32 SlotId) const
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

void ANetworkBattleGameMode::SpawnTankForPlayer(APlayerController* PlayerController)
{
	ANetworkBattlePlayerState* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkBattlePlayerState>() : nullptr;
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

		ActiveTanks[SlotId]->OnKilled.RemoveDynamic(this, &ANetworkBattleGameMode::HandleTankKilled);
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
		UE_LOG(LogTemp, Error, TEXT("NetworkBattle: failed to spawn Tank for SlotId=%d"), SlotId);
		return;
	}

	NewTank->SetSlotId(SlotId);
	NewTank->SetTeamId(NetworkPS->GetTeamId());

	PlayerController->Possess(NewTank);
	InitializeSpawnedTank(NewTank, PlayerController, NetworkPS, true);
	ActiveTanks[SlotId] = NewTank;

	UE_LOG(LogTemp, Display, TEXT("NetworkBattle: spawned Tank %s for SlotId=%d TeamId=%d"),
		*GetNameSafe(NewTank),
		SlotId,
		NetworkPS->GetTeamId());
}

void ANetworkBattleGameMode::InitializeSpawnedTank(ATank* NewTank, APlayerController* PlayerController, ANetworkBattlePlayerState* NetworkPS, bool bResetResources)
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

	NewTank->OnKilled.RemoveDynamic(this, &ANetworkBattleGameMode::HandleTankKilled);
	NewTank->OnKilled.AddDynamic(this, &ANetworkBattleGameMode::HandleTankKilled);
}

int32 ANetworkBattleGameMode::CalculateInitialAmmoForTank(const ATank* Tank) const
{
	if (!Tank)
	{
		return 0;
	}

	const float ClampedRatio = FMath::Clamp(InitialAmmoRatio, 0.0f, 1.0f);
	return FMath::Clamp(FMath::FloorToInt(Tank->MaxAmmo * ClampedRatio), 0, Tank->MaxAmmo);
}

void ANetworkBattleGameMode::HandleTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	if (!DeadTank)
	{
		return;
	}

	APlayerController* DeadPlayerController = Cast<APlayerController>(DeadTank->GetController());
	ANetworkBattlePlayerState* NetworkPS = DeadPlayerController ? DeadPlayerController->GetPlayerState<ANetworkBattlePlayerState>() : DeadTank->GetPlayerState<ANetworkBattlePlayerState>();
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

	UE_LOG(LogTemp, Display, TEXT("NetworkBattle: Tank killed. Dead=%s Killer=%s SlotId=%d"),
		*GetNameSafe(DeadTank),
		*GetNameSafe(KillerTank),
		SlotId);

	if (ATankPlayerController* DeadTankPC = Cast<ATankPlayerController>(DeadPlayerController))
	{
		DeadTankPC->ShowDeathScreenForOwner(RespawnDelay);
	}

	ScheduleRespawn(DeadPlayerController);
}

void ANetworkBattleGameMode::ScheduleRespawn(APlayerController* PlayerController)
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

void ANetworkBattleGameMode::RespawnPlayer(APlayerController* PlayerController)
{
	ANetworkBattlePlayerState* NetworkPS = PlayerController ? PlayerController->GetPlayerState<ANetworkBattlePlayerState>() : nullptr;
	if (!PlayerController || !NetworkPS)
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
		OldTank->OnKilled.RemoveDynamic(this, &ANetworkBattleGameMode::HandleTankKilled);
		if (PlayerController->GetPawn() == OldTank)
		{
			PlayerController->UnPossess();
		}
		OldTank->Destroy();
	}

	SpawnTankForPlayer(PlayerController);
}

void ANetworkBattleGameMode::RefreshConnectedPlayerCount() const
{
	if (ANetworkBattleGameState* NetworkGS = GetNetworkGameState())
	{
		const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 0;
		NetworkGS->SetConnectedPlayerCount(PlayerCount);
	}
}

ANetworkBattleGameState* ANetworkBattleGameMode::GetNetworkGameState() const
{
	return GetGameState<ANetworkBattleGameState>();
}
