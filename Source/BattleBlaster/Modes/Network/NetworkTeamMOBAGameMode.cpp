#include "Modes/Network/NetworkTeamMOBAGameMode.h"

ANetworkTeamMOBAGameMode::ANetworkTeamMOBAGameMode()
{
	TeamCount = 2;
}

int32 ANetworkTeamMOBAGameMode::ChooseTeamIdForSlot(int32 SlotId) const
{
	if (TeamCount <= 0)
	{
		return 0;
	}

	return FMath::Abs(SlotId) % TeamCount;
}
