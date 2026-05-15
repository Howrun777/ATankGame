#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"






#include "PauseMenuWidget.generated.h"
UCLASS()
class BATTLEBLASTER_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual bool Initialize() override;
	void Setup();
	void Teardown();

	// 【新增】：在蓝图里指定我们之前做好的“地图选择UI”
	UPROPERTY(EditAnywhere, Category = "UI Setup")
	TSubclassOf<class USelectMapWidget> MapSelectWidgetClass;

protected:
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_Restart;

	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_MainMenu;

	// 【新增】：绑定切换地图按钮 (名字必须和UMG里一样)
	UPROPERTY(meta = (BindWidget))
	class UButton* Btn_ChangeMap;

private:
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	// 【新增】：切换地图按钮的回调
	UFUNCTION()
	void OnChangeMapClicked();
};