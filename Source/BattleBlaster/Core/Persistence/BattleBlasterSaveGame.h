#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BattleBlasterSaveGame.generated.h"

UCLASS()
class BATTLEBLASTER_API UBattleBlasterSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UBattleBlasterSaveGame();

	// 当前闯到的关卡数
	UPROPERTY(VisibleAnywhere, Category = "Campaign")
	int32 CurrentLevelIndex;

	// 历史最高关卡记录
	UPROPERTY(VisibleAnywhere, Category = "Campaign")
	int32 BestLevelRecord;

	// 存档文件名
	// 它的值被设定为 "BattleBlasterSaveSlot"。
	// 虚幻引擎在保存时，会在你电脑硬盘的 Saved / SaveGames / 文件夹下生成一个名为 BattleBlasterSaveSlot.sav 的本地文件。
	static const FString SaveSlotName;
	// 它的值被设定为 0。
	// 这个主要用于主机平台（如 PlayStation, Xbox），
	// 主机允许一台机器上有多个不同的系统账号（比如“哥哥的账号”和“弟弟的账号”）。
	// 索引 0 通常代表当前本地主控的第一个玩家（也就是本地单机游戏的默认玩家）。
	// 在 PC 端，这个值写 0 就可以了。
	static const uint32 UserIndex;
};
