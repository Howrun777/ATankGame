#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "BuffTypes.generated.h"

// 1. 定义 9 种 Buff 外观类型 (包含随机问号)
UENUM(BlueprintType)
enum class EBuffType : uint8
{
	None,
	Heal        UMETA(DisplayName = "Health Restore (One-Time)"),
	Ammo        UMETA(DisplayName = "Infinite Ammo (Duration)"),
	Speed       UMETA(DisplayName = "Movement Speed Boost (Duration)"),
	Pierce      UMETA(DisplayName = "Bullet Pierce (Duration)"),
	Ghost       UMETA(DisplayName = "Ghost Mode (Duration)"),
	Damage      UMETA(DisplayName = "Damage Boost (Duration)"),
	DoubleShot  UMETA(DisplayName = "Double Shot (Duration)"),
	Shield      UMETA(DisplayName = "Shield (One-Time)"),
	RandomIcon  UMETA(DisplayName = "Random Buff (Visual Only)")
};

// 2. 拾取物的外观配置结构体 
USTRUCT(BlueprintType)
struct FBuffVisualData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* BuffMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* BuffMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* UIIcon; // UI上显示的图标
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BuffDuration; //buff的持续时间
};


