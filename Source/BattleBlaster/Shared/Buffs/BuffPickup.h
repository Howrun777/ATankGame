#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Shared/Buffs/BuffTypes.h" // 你自己定义的Buff类型头文件（包含EBuffType枚举和FBuffVisualData结构体）
#include "BuffPickup.generated.h"

// 前置声明（减少编译依赖，不需要在这里include完整头文件）
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * @brief 可拾取Buff实体类
 * 功能：随机生成Buff外观、玩家重叠拾取、播放特效、定时重生
 */
UCLASS()
class BATTLEBLASTER_API ABuffPickup : public AActor
{
	GENERATED_BODY()

public:
	// 构造函数：初始化组件和默认值
	ABuffPickup();

protected:
	// 游戏开始时或生成时调用
	virtual void BeginPlay() override;

	// 每帧调用
	virtual void Tick(float DeltaTime) override;

	// ================= 组件部分 =================

	// 球形碰撞体：用于检测玩家重叠（根组件）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereComp;

	// 静态网格体组件：显示Buff的3D模型
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	// Niagara粒子组件：Buff待机时的特效（比如发光、环绕粒子）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNiagaraComponent* IdleParticleComp;

	// ================= 配置数据部分（可在蓝图编辑器中修改） =================


	// 核心数据映射：Buff类型 -> 视觉数据（网格体、材质、图标）
	// 你需要在蓝图里为1-9号Buff分别配置对应的模型和材质
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup")
	TMap<EBuffType, FBuffVisualData> BuffVisualMap;

	// 拾取时播放的Niagara特效（比如爆炸、闪光）
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup|Effects")
	UNiagaraSystem* PickupEffect;

	// 拾取时播放的音效
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup|Effects")
	USoundBase* PickupSound;

	// 重生时间（单位：秒）：拾取后多久重新刷新出来
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup", meta = (MinValue = 1.0f))
	float RespawnTime = 60.0f;

	// 动画参数：Buff自身旋转速度（度/秒）
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup|Animation")
	float RotateSpeed = 90.0f;

	// 动画参数：上下浮动速度
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup|Animation")
	float HoverSpeed = 2.0f;

	// 动画参数：上下浮动幅度（单位：厘米）
	UPROPERTY(EditDefaultsOnly, Category = "Buff Setup|Animation")
	float HoverAmplitude = 10.0f;

private:
	// ================= 内部状态变量 =================

	// 当前Buff显示的类型（决定了外观）
	EBuffType CurrentVisualType;

	// 记录初始位置：用于计算上下浮动（否则浮动会基于当前位置累积偏移）
	FVector BaseLocation;

	// 计时器句柄：用于控制Buff重生倒计时
	FTimerHandle RespawnTimerHandle;

	// ================= 核心逻辑函数 =================

	// 随机初始化Buff外观：从1-9中随机选一个类型并应用对应的网格体/材质
	void InitializeRandomBuff();

	// 隐藏Buff并开始重生倒计时
	void HideAndStartRespawnTimer();

	// 重生Buff：重新随机外观并恢复显示
	void RespawnBuff();

	// 重叠事件回调：当有物体进入碰撞体时调用
	// UFUNCTION()宏是必须的，否则反射系统无法绑定这个回调
	UFUNCTION()
	void OnOverlapBegin(
		UPrimitiveComponent* OverlappedComp,  // 触发事件的组件（这里是SphereComp）
		AActor* OtherActor,                   // 进入碰撞体的另一个Actor
		UPrimitiveComponent* OtherComp,       // 另一个Actor的组件
		int32 OtherBodyIndex,                 // 另一个组件的Body索引（用于复杂骨骼模型）
		bool bFromSweep,                      // 是否是扫过（Sweep）触发的
		const FHitResult& SweepResult         // 扫过结果的详细信息
	);
};


/*
* 配置拾取物 (BP_BuffPickup)：
创建一个基于 ABuffPickup 的蓝图。
选中它的根节点，看右侧细节面板有个 Buff Visual Map。
点击 + 号 9 次。把键值分别选为 9 种枚举类型（Heal, Speed, RandomIcon 等等）。
在对应的 Value 里面，填入你做好的圆饼模型 (Mesh)、带图案的发光材质 (Material) 和 UI图片 (Icon)。
(注：发光材质只需要在 Material 的 Emissive Color 里连上乘法即可；悬浮和旋转 C++ 已经帮你自动做好了！)
*/
