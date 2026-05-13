#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"






#include "ScreenMessage.generated.h"


UCLASS()
class BATTLEBLASTER_API UScreenMessage : public UUserWidget
{
	GENERATED_BODY()
public:
	/*meta = (BindWidget) 元数据标签是专门用于 UMG UI 开发的核心元数据*/
	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UTextBlock* MessageTextBlock;

	// 设置文本内容
	UFUNCTION(BlueprintCallable)
	void SetMessageText(FString Message);
	// 设置文本颜色
	UFUNCTION(BlueprintCallable)
	void SetMessageColor(FLinearColor Color);
};
