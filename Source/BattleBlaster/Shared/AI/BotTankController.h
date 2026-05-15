// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BattleBlaster.h"
#include "BotTankController.generated.h"

/**
 * AI 控制器的坦克
 */
UCLASS()
class BATTLEBLASTER_API ABotTankController : public AAIController
{
	GENERATED_BODY()
	
protected:
    virtual void BeginPlay() override;

public:
    // 简单的时间间隔来刷新移动方向
    float DirectionChangeInterval = 2.0f;

    // 下一次改变移动方向的时间
    float NextDirectionChangeTime = 0.0f;

    // 当前的移动输入向量
    FVector2D CurrentMoveInput = FVector2D::Zero();
};
