#include "Modes/Network/NetworkGameModeBase.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkGameStateBase.h"
#include "Modes/Network/NetworkPlayerControllerBase.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Shared/AI/AIBotPlayerController.h"
#include "Shared/Combat/HealthComponent.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/Pawns/Tank.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANetworkGameModeBase::ANetworkGameModeBase()
{
	GameStateClass = ANetworkGameStateBase::StaticClass();
	PlayerStateClass = ANetworkPlayerStateBase::StaticClass();
	PlayerControllerClass = ANetworkPlayerControllerBase::StaticClass();
	DefaultPawnClass = nullptr;

	MaxNetworkPlayers = 4;
	AICount = 0;
	bUseTaggedPlayerStarts = true;
	InitialAmmoRatio = 0.5f;
	RespawnDelay = 3.0f;
	RespawnHealthPercent = 1.0f;

	static ConstructorHelpers::FClassFinder<ATank> DefaultTankBlueprint(TEXT("/Game/Blueprints/Tanks/BP_TankGreen"));
	if (DefaultTankBlueprint.Succeeded())
	{
		TankClass = DefaultTankBlueprint.Class;
	}
	else
	{
		TankClass = ATank::StaticClass();
	}

	AIControllerClass = AAIBotPlayerController::StaticClass();
	NetworkAIDifficulty = EAIDifficulty::Normal;
}

void ANetworkGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString MaxPlayersOption = UGameplayStatics::ParseOption(Options, TEXT("MaxPlayers"));
	if (!MaxPlayersOption.IsEmpty())
	{
		MaxNetworkPlayers = FMath::Clamp(FCString::Atoi(*MaxPlayersOption), 1, 8);
	}

	const FString AICountOption = UGameplayStatics::ParseOption(Options, TEXT("AICount"));
	if (!AICountOption.IsEmpty())
	{
		AICount = FMath::Clamp(FCString::Atoi(*AICountOption), 0, FMath::Max(0, MaxNetworkPlayers - 1));
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

	if (HasAuthority() && AICount > 0)
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ANetworkGameModeBase::SpawnConfiguredAIPlayers);
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase BeginPlay. MaxNetworkPlayers=%d AICount=%d"), MaxNetworkPlayers, AICount);
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

			InitializePlayerIdentity(NewPlayer, SlotId, false);
		}
	}

	SpawnTankForController(NewPlayer);
	RefreshConnectedPlayerCount();
}

bool ANetworkGameModeBase::UsesTeamDamageRules() const
{
	return false;
}

bool ANetworkGameModeBase::AreTeamIdsHostile(int32 AttackerTeamId, int32 VictimTeamId) const
{
	if (AttackerTeamId < 0 || VictimTeamId < 0)
	{
		return true;
	}

	return AttackerTeamId != VictimTeamId;
}

bool ANetworkGameModeBase::CanTankDamageTank(const ATank* AttackerTank, const ATank* VictimTank) const
{
	if (!AttackerTank || !VictimTank)
	{
		return true;
	}

	if (AttackerTank == VictimTank)
	{
		return false;
	}

	if (!UsesTeamDamageRules())
	{
		return true;
	}

	return AreTeamIdsHostile(AttackerTank->GetTeamId(), VictimTank->GetTeamId());
}

int32 ANetworkGameModeBase::AllocateSlotId() const
{
	const AGameStateBase* GS = GameState;
	if (!GS)
	{
		return -1;
	}

	for (int32 Candidate = 0; Candidate < GetMaxHumanPlayerSlots(); ++Candidate)
	{
		if (!IsSlotOccupied(Candidate))
		{
			return Candidate;
		}
	}

	return -1;
}

void ANetworkGameModeBase::InitializePlayerIdentity(AController* Controller, int32 SlotId, bool bAIPlayer) const
{
	ANetworkPlayerStateBase* NetworkPS = Controller ? Controller->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!NetworkPS)
	{
		return;
	}

	NetworkPS->SetSlotId(SlotId);
	NetworkPS->SetTeamId(ChooseTeamIdForSlot(SlotId));
	NetworkPS->SetReady(bAIPlayer);
	NetworkPS->SetAIPlayer(bAIPlayer);

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: assigned SlotId=%d TeamId=%d AI=%s to %s"),
		NetworkPS->GetSlotId(),
		NetworkPS->GetTeamId(),
		bAIPlayer ? TEXT("true") : TEXT("false"),
		*GetNameSafe(Controller));
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

void ANetworkGameModeBase::SpawnTankForController(AController* Controller)
{
	ANetworkPlayerStateBase* NetworkPS = Controller ? Controller->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!Controller || !NetworkPS)
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
			Controller->Possess(ActiveTanks[SlotId]);
			InitializeSpawnedTank(ActiveTanks[SlotId], Controller, NetworkPS, false);
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
	SpawnParams.Owner = Controller;
	SpawnParams.Instigator = Controller->GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ATank* NewTank = GetWorld()->SpawnActor<ATank>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
	if (!NewTank)
	{
		UE_LOG(LogTemp, Error, TEXT("NetworkGameModeBase: failed to spawn Tank for SlotId=%d"), SlotId);
		return;
	}

	NewTank->SetSlotId(SlotId);
	NewTank->SetTeamId(NetworkPS->GetTeamId());

	Controller->Possess(NewTank);
	InitializeSpawnedTank(NewTank, Controller, NetworkPS, true);
	ActiveTanks[SlotId] = NewTank;

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: spawned Tank %s for SlotId=%d TeamId=%d"),
		*GetNameSafe(NewTank),
		SlotId,
		NetworkPS->GetTeamId());
}

void ANetworkGameModeBase::InitializeSpawnedTank(ATank* NewTank, AController* Controller, ANetworkPlayerStateBase* NetworkPS, bool bResetResources)
{
	if (!NewTank || !Controller || !NetworkPS)
	{
		return;
	}

	NewTank->SetOwner(Controller);
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

	if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(Controller))
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

	AController* DeadController = DeadTank->GetController();
	ANetworkPlayerStateBase* NetworkPS = DeadController ? DeadController->GetPlayerState<ANetworkPlayerStateBase>() : DeadTank->GetPlayerState<ANetworkPlayerStateBase>();
	if (!DeadController && NetworkPS)
	{
		DeadController = Cast<AController>(NetworkPS->GetOwner());
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

	if (ATankPlayerController* DeadTankPC = Cast<ATankPlayerController>(DeadController))
	{
		DeadTankPC->ShowDeathScreenForOwner(RespawnDelay);
	}

	if (ShouldRespawnPlayer(NetworkPS))
	{
		ScheduleRespawn(DeadController);
	}

	CheckNetworkGameOver();
}

void ANetworkGameModeBase::CheckNetworkGameOver()
{
}

void ANetworkGameModeBase::ScheduleRespawn(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	if (RespawnDelay <= 0.0f)
	{
		RespawnPlayer(Controller);
		return;
	}

	FTimerHandle RespawnTimerHandle;
	FTimerDelegate RespawnDelegate;
	TWeakObjectPtr<AController> WeakController(Controller);
	RespawnDelegate.BindWeakLambda(this, [this, WeakController]()
	{
		if (AController* ValidController = WeakController.Get())
		{
			RespawnPlayer(ValidController);
		}
	});

	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

void ANetworkGameModeBase::RespawnPlayer(AController* Controller)
{
	ANetworkPlayerStateBase* NetworkPS = Controller ? Controller->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!Controller || !NetworkPS)
	{
		return;
	}

	if (!ShouldRespawnPlayer(NetworkPS))
	{
		return;
	}

	if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(Controller))
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
		OldTank = Cast<ATank>(Controller->GetPawn());
	}

	if (OldTank && IsValid(OldTank))
	{
		OldTank->OnKilled.RemoveDynamic(this, &ANetworkGameModeBase::HandleTankKilled);
		if (Controller->GetPawn() == OldTank)
		{
			Controller->UnPossess();
		}
		OldTank->Destroy();
	}

	SpawnTankForController(Controller);
}

void ANetworkGameModeBase::RefreshConnectedPlayerCount() const
{
	if (ANetworkGameStateBase* NetworkGS = GetNetworkGameState())
	{
		const int32 PlayerCount = GameState ? GameState->PlayerArray.Num() : 0;
		NetworkGS->SetConnectedPlayerCount(PlayerCount);
	}
}

void ANetworkGameModeBase::SpawnConfiguredAIPlayers()
{
	if (!HasAuthority() || AICount <= 0)
	{
		return;
	}

	const int32 FirstAISlot = GetMaxHumanPlayerSlots();
	for (int32 SlotId = FirstAISlot; SlotId < MaxNetworkPlayers; ++SlotId)
	{
		SpawnAIForSlot(SlotId);
	}

	RefreshConnectedPlayerCount();
}

void ANetworkGameModeBase::SpawnAIForSlot(int32 SlotId)
{
	if (!GetWorld() || SlotId < 0 || SlotId >= MaxNetworkPlayers || IsSlotOccupied(SlotId))
	{
		return;
	}

	TSubclassOf<AAIBotPlayerController> ClassToSpawn = AIControllerClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = AAIBotPlayerController::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAIBotPlayerController* AIController = GetWorld()->SpawnActor<AAIBotPlayerController>(
		ClassToSpawn,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);

	if (!AIController)
	{
		UE_LOG(LogTemp, Warning, TEXT("NetworkGameModeBase: failed to spawn AI controller for SlotId=%d"), SlotId);
		return;
	}

	InitializePlayerIdentity(AIController, SlotId, true);

	if (ANetworkPlayerStateBase* NetworkPS = AIController->GetPlayerState<ANetworkPlayerStateBase>())
	{
		NetworkPS->SetPlayerName(FString::Printf(TEXT("AI_P%d"), SlotId + 1));
	}

	AIController->ApplyDifficultySettings(NetworkAIDifficulty);
	SpawnTankForController(AIController);

	UE_LOG(LogTemp, Display, TEXT("NetworkGameModeBase: spawned AI player at SlotId=%d"), SlotId);
}

int32 ANetworkGameModeBase::GetMaxHumanPlayerSlots() const
{
	return FMath::Clamp(MaxNetworkPlayers - AICount, 1, MaxNetworkPlayers);
}

bool ANetworkGameModeBase::IsSlotOccupied(int32 SlotId) const
{
	const AGameStateBase* GS = GameState;
	if (!GS)
	{
		return false;
	}

	for (APlayerState* PlayerState : GS->PlayerArray)
	{
		const ANetworkPlayerStateBase* NetworkPS = Cast<ANetworkPlayerStateBase>(PlayerState);
		if (NetworkPS && NetworkPS->GetSlotId() == SlotId)
		{
			return true;
		}
	}

	return false;
}

ANetworkGameStateBase* ANetworkGameModeBase::GetNetworkGameState() const
{
	return GetGameState<ANetworkGameStateBase>();
}
