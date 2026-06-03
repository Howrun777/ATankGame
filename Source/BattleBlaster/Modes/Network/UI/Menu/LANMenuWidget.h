#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "LANMenuWidget.generated.h"

class UButton;

UCLASS()
class BATTLEBLASTER_API ULANMenuWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	UButton* HostButton = nullptr;

	UPROPERTY()
	UButton* JoinButton = nullptr;

	UPROPERTY()
	UButton* BackButton = nullptr;

	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();
};
