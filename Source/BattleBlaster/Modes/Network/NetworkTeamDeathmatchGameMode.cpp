#include "Modes/Network/NetworkTeamDeathmatchGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Modes/Network/NetworkPlayerStateBase.h"
#include "Modes/Network/NetworkTeamDeathmatchGameState.h"
#include "Shared/Pawns/Tank.h"

ANetworkTeamDeathmatchGameMode::ANetworkTeamDeathmatchGameMode()
{
	GameStateClass = ANetworkTeamDeathmatchGameState::StaticClass();
	TargetScore = 7;
	TeamCount = 2;
}

void ANetworkTeamDeathmatchGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	const FString TargetScoreOption = UGameplayStatics::ParseOption(Options, TEXT("TargetScore"));
	if (!TargetScoreOption.IsEmpty())
	{
		TargetScore = FMath::Max(1, FCString::Atoi(*TargetScoreOption));
	}

	const FString TeamCountOption = UGameplayStatics::ParseOption(Options, TEXT("TeamCount"));
	if (!TeamCountOption.IsEmpty())
	{
		TeamCount = FMath::Clamp(FCString::Atoi(*TeamCountOption), 2, 4);
	}
}

void ANetworkTeamDeathmatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ANetworkTeamDeathmatchGameState* TeamGS = GetTeamDeathmatchGameState())
	{
		TeamGS->InitializeTeamScores(TeamCount, TargetScore);
	}

	GetWorldTimerManager().SetTimer(
		MatchElapsedTimerHandle,
		this,
		&ANetworkTeamDeathmatchGameMode::UpdateMatchElapsedTime,
		1.0f,
		true);
}

int32 ANetworkTeamDeathmatchGameMode::ChooseTeamIdForSlot(int32 SlotId) const
{
	if (TeamCount <= 0)
	{
		return 0;
	}

	return FMath::Abs(SlotId) % TeamCount;
}

bool ANetworkTeamDeathmatchGameMode::ShouldRespawnPlayer(ANetworkPlayerStateBase* PlayerState) const
{
	const ANetworkTeamDeathmatchGameState* TeamGS = GetGameState<ANetworkTeamDeathmatchGameState>();
	return PlayerState != nullptr && (!TeamGS || !TeamGS->IsMatchOver());
}

void ANetworkTeamDeathmatchGameMode::HandleNetworkTankKilled(ATank* DeadTank, ATank* KillerTank)
{
	AddTeamDeathmatchScore(DeadTank, KillerTank);
	Super::HandleNetworkTankKilled(DeadTank, KillerTank);
}

void ANetworkTeamDeathmatchGameMode::CheckNetworkGameOver()
{
	ANetworkTeamDeathmatchGameState* TeamGS = GetTeamDeathmatchGameState();
	if (!TeamGS || TeamGS->IsMatchOver())
	{
		return;
	}

	for (int32 TeamId = 0; TeamId < TeamGS->TeamScores.Num(); ++TeamId)
	{
		if (TeamGS->GetTeamScore(TeamId) >= TeamGS->TargetScore)
		{
			TeamGS->SetWinningTeamId(TeamId);
			HandleTeamDeathmatchEnded(TeamId);
			break;
		}
	}
}

ANetworkTeamDeathmatchGameState* ANetworkTeamDeathmatchGameMode::GetTeamDeathmatchGameState() const
{
	return GetGameState<ANetworkTeamDeathmatchGameState>();
}

void ANetworkTeamDeathmatchGameMode::AddTeamDeathmatchScore(ATank* DeadTank, ATank* KillerTank)
{
	ANetworkTeamDeathmatchGameState* TeamGS = GetTeamDeathmatchGameState();
	if (!TeamGS || TeamGS->IsMatchOver() || !DeadTank)
	{
		return;
	}

	if (KillerTank && KillerTank != DeadTank)
	{
		const int32 KillerTeamId = KillerTank->GetTeamId();
		const int32 VictimTeamId = DeadTank->GetTeamId();
		if (AreTeamIdsHostile(KillerTeamId, VictimTeamId) && TeamGS->TeamScores.IsValidIndex(KillerTeamId))
		{
			TeamGS->AddTeamScore(KillerTeamId, 1);
			if (TeamGS->GetTeamScore(KillerTeamId) >= TeamGS->TargetScore)
			{
				TeamGS->SetWinningTeamId(KillerTeamId);
				HandleTeamDeathmatchEnded(KillerTeamId);
			}
		}
		return;
	}

	const int32 VictimTeamId = DeadTank->GetTeamId();
	if (TeamGS->TeamScores.IsValidIndex(VictimTeamId))
	{
		TeamGS->AddTeamScore(VictimTeamId, -1);
	}
}

void ANetworkTeamDeathmatchGameMode::HandleTeamDeathmatchEnded(int32 WinnerTeamId)
{
	GetWorldTimerManager().ClearTimer(MatchElapsedTimerHandle);

	for (ATank* Tank : ActiveTanks)
	{
		if (Tank && Tank->GetIsAlive())
		{
			Tank->SetPlayerEnabled(false);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("NetworkTeamDeathmatch: Match ended. WinnerTeamId=%d"), WinnerTeamId);
}

void ANetworkTeamDeathmatchGameMode::UpdateMatchElapsedTime()
{
	if (ANetworkTeamDeathmatchGameState* TeamGS = GetTeamDeathmatchGameState())
	{
		TeamGS->IncrementMatchElapsedSeconds();
	}
}
