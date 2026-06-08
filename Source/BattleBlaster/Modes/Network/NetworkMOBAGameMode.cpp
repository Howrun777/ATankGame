#include "Modes/Network/NetworkMOBAGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkMOBAGameState.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Shared/Controllers/TankPlayerController.h"
#include "Shared/Pawns/Tank.h"

ANetworkMOBAGameMode::ANetworkMOBAGameMode()
{
	GameStateClass = ANetworkMOBAGameState::StaticClass();
	TeamCount = 4;
	bAssumeCoreAliveUntilRegistered = true;
	RespawnDelay = 4.0f;
}

void ANetworkMOBAGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString TeamCountOption = UGameplayStatics::ParseOption(Options, TEXT("TeamCount"));
	if (!TeamCountOption.IsEmpty())
	{
		TeamCount = FMath::Clamp(FCString::Atoi(*TeamCountOption), 1, 8);
	}
	else
	{
		TeamCount = FMath::Clamp(MaxNetworkPlayers, 1, 8);
	}
}

void ANetworkMOBAGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ANetworkMOBAGameState* MobaGS = GetMOBAGameState())
	{
		MobaGS->InitializeMOBAState(TeamCount);
	}

	GetWorldTimerManager().SetTimer(
		MatchElapsedTimerHandle,
		this,
		&ANetworkMOBAGameMode::UpdateMatchElapsedTime,
		1.0f,
		true);
}

void ANetworkMOBAGameMode::RegisterCoreForTeam(int32 TeamId)
{
	if (ANetworkMOBAGameState* MobaGS = GetMOBAGameState())
	{
		MobaGS->RegisterCoreForTeam(TeamId);
	}
}

void ANetworkMOBAGameMode::NotifyCoreDestroyedForTeam(int32 TeamId)
{
	if (ANetworkMOBAGameState* MobaGS = GetMOBAGameState())
	{
		MobaGS->MarkCoreDestroyedForTeam(TeamId);
	}
}

int32 ANetworkMOBAGameMode::ChooseTeamIdForSlot(int32 SlotId) const
{
	return SlotId;
}

bool ANetworkMOBAGameMode::ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const
{
	if (!PlayerState)
	{
		return false;
	}

	const ANetworkMOBAGameState* MobaGS = GetGameState<ANetworkMOBAGameState>();
	if (MobaGS && MobaGS->IsMatchOver())
	{
		return false;
	}

	return HasCoreAliveForTeam(PlayerState->GetTeamId());
}

void ANetworkMOBAGameMode::HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	Super::HandleNetworkTankKilled(DeadTank, KillerTank);

	ANetworkPlayerStateBase* DeadPS = DeadTank ? DeadTank->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!DeadPS)
	{
		return;
	}

	const int32 DeadTeamId = DeadPS->GetTeamId();
	if (!HasCoreAliveForTeam(DeadTeamId))
	{
		if (ANetworkMOBAGameState* MobaGS = GetMOBAGameState())
		{
			MobaGS->SetTeamEliminated(DeadTeamId, true);
		}

		if (ATankPlayerController* DeadPC = Cast<ATankPlayerController>(DeadPS->GetOwner()))
		{
			DeadPC->HideDeathScreenForOwner();
			DeadPC->ShowEliminatedScreen();
		}
	}

	CheckNetworkGameOver();
}

void ANetworkMOBAGameMode::CheckNetworkGameOver()
{
	ANetworkMOBAGameState* MobaGS = GetMOBAGameState();
	if (!MobaGS || MobaGS->IsMatchOver())
	{
		return;
	}

	int32 RemainingTeamCount = 0;
	int32 WinnerTeamId = -1;
	for (int32 TeamId = 0; TeamId < MobaGS->bTeamEliminated.Num(); ++TeamId)
	{
		if (!MobaGS->IsTeamEliminated(TeamId))
		{
			++RemainingTeamCount;
			WinnerTeamId = TeamId;
		}
	}

	if (RemainingTeamCount == 1 && WinnerTeamId >= 0)
	{
		MobaGS->SetWinningTeamId(WinnerTeamId);
		GetWorldTimerManager().ClearTimer(MatchElapsedTimerHandle);

		for (ATank* Tank : ActiveTanks)
		{
			if (Tank && Tank->GetIsAlive())
			{
				Tank->SetPlayerEnabled(false);
			}
		}

		UE_LOG(LogTemp, Display, TEXT("NetworkMOBA: Match ended. WinnerTeamId=%d"), WinnerTeamId);
	}
}

void ANetworkMOBAGameMode::RespawnPlayer(AController* Controller)
{
	ANetworkPlayerStateBase* NetworkPS = Controller ? Controller->GetPlayerState<ANetworkPlayerStateBase>() : nullptr;
	if (!ShouldRespawnPlayer(NetworkPS))
	{
		if (ATankPlayerController* TankPC = Cast<ATankPlayerController>(Controller))
		{
			TankPC->HideDeathScreenForOwner();
			TankPC->ShowEliminatedScreen();
		}
		return;
	}

	Super::RespawnPlayer(Controller);
}

ANetworkMOBAGameState* ANetworkMOBAGameMode::GetMOBAGameState() const
{
	return GetGameState<ANetworkMOBAGameState>();
}

bool ANetworkMOBAGameMode::HasCoreAliveForTeam(int32 TeamId) const
{
	const ANetworkMOBAGameState* MobaGS = GetGameState<ANetworkMOBAGameState>();
	if (!MobaGS || TeamId < 0)
	{
		return bAssumeCoreAliveUntilRegistered;
	}

	if (!MobaGS->AliveCoreCountsByTeam.IsValidIndex(TeamId))
	{
		return bAssumeCoreAliveUntilRegistered;
	}

	const bool bNoCoreRegisteredYet = MobaGS->AliveCoreCountsByTeam.Num() > 0;
	if (bAssumeCoreAliveUntilRegistered && bNoCoreRegisteredYet)
	{
		bool bAnyCoreRegistered = false;
		for (int32 Count : MobaGS->AliveCoreCountsByTeam)
		{
			if (Count > 0)
			{
				bAnyCoreRegistered = true;
				break;
			}
		}

		if (!bAnyCoreRegistered)
		{
			return true;
		}
	}

	return MobaGS->HasCoreAliveForTeam(TeamId);
}

void ANetworkMOBAGameMode::UpdateMatchElapsedTime()
{
	if (ANetworkMOBAGameState* MobaGS = GetMOBAGameState())
	{
		MobaGS->IncrementMatchElapsedSeconds();
	}
}
