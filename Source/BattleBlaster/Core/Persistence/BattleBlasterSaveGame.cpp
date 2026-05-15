#include "Core/Persistence/BattleBlasterSaveGame.h"

const FString UBattleBlasterSaveGame::SaveSlotName = TEXT("BattleBlasterSaveSlot");
const uint32 UBattleBlasterSaveGame::UserIndex = 0;


//这意味着如果玩家第一次打开游戏（没有存档）
//系统创建一个新的存档对象时，默认就是从第 1 关开始。
UBattleBlasterSaveGame::UBattleBlasterSaveGame()
{
	CurrentLevelIndex = 1;
	BestLevelRecord = 1;
}
