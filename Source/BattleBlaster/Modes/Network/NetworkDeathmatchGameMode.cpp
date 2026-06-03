#include "Modes/Network/NetworkDeathmatchGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkDeathmatchGameState.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Shared/Pawns/Tank.h"

ANetworkDeathmatchGameMode::ANetworkDeathmatchGameMode()
{
	GameStateClass = ANetworkDeathmatchGameState::StaticClass();
	TargetScore = 7;
}

void ANetworkDeathmatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString TargetScoreOption = UGameplayStatics::ParseOption(Options, TEXT("TargetScore"));
	if (!TargetScoreOption.IsEmpty())
	{
		TargetScore = FMath::Max(1, FCString::Atoi(*TargetScoreOption));
	}
}

void ANetworkDeathmatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ANetworkDeathmatchGameState* DeathmatchGS = GetDeathmatchGameState())
	{
		DeathmatchGS->InitializeDeathmatchScores(MaxNetworkPlayers, TargetScore);
	}

	GetWorldTimerManager().SetTimer(
		MatchElapsedTimerHandle,
		this,
		&ANetworkDeathmatchGameMode::UpdateMatchElapsedTime,
		1.0f,
		true);
}

bool ANetworkDeathmatchGameMode::ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const
{
	const ANetworkDeathmatchGameState* DeathmatchGS = GetGameState<ANetworkDeathmatchGameState>();
	return PlayerState != nullptr && (!DeathmatchGS || !DeathmatchGS->IsMatchOver());
}

void ANetworkDeathmatchGameMode::HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	AddDeathmatchScore(DeadTank, KillerTank);
	Super::HandleNetworkTankKilled(DeadTank, KillerTank);
}

void ANetworkDeathmatchGameMode::CheckNetworkGameOver()
{
	ANetworkDeathmatchGameState* DeathmatchGS = GetDeathmatchGameState();
	if (!DeathmatchGS || DeathmatchGS->IsMatchOver())
	{
		return;
	}

	for (int32 SlotId = 0; SlotId < DeathmatchGS->PlayerScores.Num(); ++SlotId)
	{
		if (DeathmatchGS->GetPlayerScore(SlotId) >= DeathmatchGS->TargetScore)
		{
			DeathmatchGS->SetWinnerSlotId(SlotId);
			HandleDeathmatchEnded(SlotId);
			UE_LOG(LogTemp, Display, TEXT("NetworkDeathmatch: SlotId=%d wins with score %d / %d"),
				SlotId,
				DeathmatchGS->GetPlayerScore(SlotId),
				DeathmatchGS->TargetScore);
			break;
		}
	}
}

ANetworkDeathmatchGameState* ANetworkDeathmatchGameMode::GetDeathmatchGameState() const
{
	return GetGameState<ANetworkDeathmatchGameState>();
}

void ANetworkDeathmatchGameMode::AddDeathmatchScore(ATank* DeadTank, ATank* KillerTank)
{
	ANetworkDeathmatchGameState* DeathmatchGS = GetDeathmatchGameState();
	if (!DeathmatchGS || DeathmatchGS->IsMatchOver() || !DeadTank)
	{
		return;
	}

	const int32 VictimSlotId = DeadTank->GetSlotId();
	const int32 KillerSlotId = KillerTank ? KillerTank->GetSlotId() : INDEX_NONE;

	if (KillerTank && KillerTank != DeadTank && DeathmatchGS->PlayerScores.IsValidIndex(KillerSlotId))
	{
		DeathmatchGS->AddPlayerScore(KillerSlotId, 1);
		if (DeathmatchGS->GetPlayerScore(KillerSlotId) >= DeathmatchGS->TargetScore)
		{
			DeathmatchGS->SetWinnerSlotId(KillerSlotId);
			HandleDeathmatchEnded(KillerSlotId);
		}

		UE_LOG(LogTemp, Display, TEXT("NetworkDeathmatch: SlotId=%d scored. Score=%d"),
			KillerSlotId,
			DeathmatchGS->GetPlayerScore(KillerSlotId));
		return;
	}

	if (DeathmatchGS->PlayerScores.IsValidIndex(VictimSlotId))
	{
		DeathmatchGS->AddPlayerScore(VictimSlotId, -1);
		UE_LOG(LogTemp, Display, TEXT("NetworkDeathmatch: SlotId=%d lost one point. Score=%d"),
			VictimSlotId,
			DeathmatchGS->GetPlayerScore(VictimSlotId));
	}
}

void ANetworkDeathmatchGameMode::HandleDeathmatchEnded(int32 WinnerSlotId)
{
	GetWorldTimerManager().ClearTimer(MatchElapsedTimerHandle);

	for (ATank* Tank : ActiveTanks)
	{
		if (Tank && Tank->GetIsAlive())
		{
			Tank->SetPlayerEnabled(false);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkDeathmatch: Match ended. WinnerSlotId=%d"), WinnerSlotId);
}

void ANetworkDeathmatchGameMode::UpdateMatchElapsedTime()
{
	if (ANetworkDeathmatchGameState* DeathmatchGS = GetDeathmatchGameState())
	{
		DeathmatchGS->IncrementMatchElapsedSeconds();
	}
}
