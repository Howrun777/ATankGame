#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class BATTLEBLASTER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()


public:
	// 【新增】在编辑器里把 WBP_MutiPlayerMenuWidget 拖进去
	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<UUserWidget> MultiplayerMenuClass;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<class UTankStageStartWidget> SinglePlayerMenuClass;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<class UGameSettingsMenuWidget> SettingsMenuClass;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	TSubclassOf<class UNetworkModeSelectWidget> NetworkMenuClass;

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void OpenNetworkMenu();

protected:
	// meta = (BindWidget) 必须确保蓝图中按钮的名字与此处变量名完全一致
	UPROPERTY(meta = (BindWidget))
	UButton* BtnSinglePlayer;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnTwoPlayers;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnSettings;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* BtnNetworkGame;

	UPROPERTY(meta = (BindWidget))
	UButton* BtnQuitGame;

	// 必须重写此函数来绑定点击事件
	virtual void NativeConstruct() override;

	// 点击回调函数
	UFUNCTION()
	void OnSinglePlayerClicked();

	UFUNCTION()
	void OnMultiPlayersClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnNetworkGameClicked();

	UFUNCTION()
	void OnQuitClicked();
};
