#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "NetworkJoinMessageWidget.generated.h"

class UCanvasPanel;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API UNetworkJoinMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNetworkJoinMessageWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Network|Join Message")
	void ShowMessage(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Network|Join Message")
	void HideMessage();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MessageTextBlock = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Join Message|Style")
	FLinearColor MessageColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Join Message|Style", meta = (ClampMin = "1"))
	int32 FontSize = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Join Message|Style", meta = (ClampMin = "0.1"))
	float DisplayDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Join Message|Style")
	FVector2D ScreenOffset = FVector2D(48.0f, 0.0f);

private:
	UPROPERTY()
	UCanvasPanel* GeneratedRootCanvas = nullptr;

	FString CurrentMessage;
	FTimerHandle HideTimerHandle;

	void BuildDefaultWidgetTree();
	bool HasBlueprintLayout() const;
	void ApplyMessageStyle();
};
