#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BattleBlasterGameInstance.h"
#include "BattleBlasterHistorySaveGame.generated.h"

/**
 * 多人死斗历史战绩池存档
 * 持久化前 50 条记录，进程重启不丢失
 */
UCLASS()
class BATTLEBLASTER_API UBattleBlasterHistorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UBattleBlasterHistorySaveGame();

	/** 历史记录（最多 50 条），按评分降序 */
	UPROPERTY(VisibleAnywhere, Category = "MultiBattle")
	TArray<FMultiBattleHistoryEntry> MultiBattleHistory;

	/** 下一条记录的 SequenceId（用于同分后来者居上） */
	UPROPERTY(VisibleAnywhere, Category = "MultiBattle")
	int32 NextSequenceId = 0;

	static const FString SaveSlotName;
	static const uint32 UserIndex;
};
