#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BuffListWidget.generated.h"

class UVerticalBox;
class UBuffSlotWidget;
class ATankPlayerController; // 新增：前置声明玩家控制器

UCLASS()
class BATTLEBLASTER_API UBuffListWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	// 开启每帧更新
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 绑定的垂直框：用来容纳所有的子槽位
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* BuffContainer;


	// 在蓝图里配置上一步写好的单条 Buff UI 蓝图类
	UPROPERTY(EditAnywhere, Category = "UI Setup")
	TSubclassOf<UBuffSlotWidget> BuffSlotClass;

private:
	// 新增：记录这个UI所属的玩家控制器
	UPROPERTY()
	ATankPlayerController* OwnerPlayerController = nullptr;
	// 执行列表刷新的内部函数
	void UpdateBuffList();
public:
	// 新增：给外部调用的初始化函数，绑定所属玩家
	UFUNCTION(BlueprintCallable, Category = "UI|Buff")
	void InitBuffUI(ATankPlayerController* InOwnerController);
};