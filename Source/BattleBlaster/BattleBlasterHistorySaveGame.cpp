#include "BattleBlasterHistorySaveGame.h"

const FString UBattleBlasterHistorySaveGame::SaveSlotName = TEXT("BattleBlasterHistorySlot");
const uint32 UBattleBlasterHistorySaveGame::UserIndex = 0;

UBattleBlasterHistorySaveGame::UBattleBlasterHistorySaveGame()
{
	MultiBattleHistory.Empty();
	NextSequenceId = 0;
}
