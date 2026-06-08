#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "NetworkModeSelectWidget.generated.h"

class ULANMenuWidget;
class UButton;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API UNetworkModeSelectWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Menu")
	TSubclassOf<ULANMenuWidget> LANMenuWidgetClass;

private:
	UPROPERTY()
	UButton* LANButton = nullptr;

	UPROPERTY()
	UButton* ServerButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	UPROPERTY()
	UTextBlock* StatusText = nullptr;

	UFUNCTION()
	void HandleLANClicked();

	UFUNCTION()
	void HandleServerClicked();
};
