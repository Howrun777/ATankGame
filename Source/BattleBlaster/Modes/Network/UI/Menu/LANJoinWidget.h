#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "LANJoinWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API ULANJoinWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	UEditableTextBox* IPTextBox = nullptr;

	UPROPERTY()
	UEditableTextBox* PortTextBox = nullptr;

	UPROPERTY()
	UTextBlock* StatusText = nullptr;

	UPROPERTY()
	UButton* JoinButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	UFUNCTION()
	void HandleJoinClicked();
};
