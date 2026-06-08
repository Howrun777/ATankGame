#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/NetworkMOBAGameMode.h"
#include "NetworkTeamMOBAGameMode.generated.h"

UCLASS()
class BATTLEBLASTER_API ANetworkTeamMOBAGameMode : public ANetworkMOBAGameMode
{
	GENERATED_BODY()

public:
	ANetworkTeamMOBAGameMode();

protected:
	virtual int32 ChooseTeamIdForSlot(int32 SlotId) const override;
};
