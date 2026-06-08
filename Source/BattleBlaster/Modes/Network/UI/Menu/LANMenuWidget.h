#pragma once

#include "CoreMinimal.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "LANMenuWidget.generated.h"

class UButton;
class ULANHostSettingsWidget;
class ULANJoinWidget;

UCLASS()
class BATTLEBLASTER_API ULANMenuWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Menu")
	TSubclassOf<ULANHostSettingsWidget> HostSettingsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Menu")
	TSubclassOf<ULANJoinWidget> JoinWidgetClass;

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
