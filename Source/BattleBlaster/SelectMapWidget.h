#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"





#include "SelectMapWidget.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;
class UBorder; //用于实现边缘高亮

// 定义一个结构体，用来存放一张地图的信息
USTRUCT(BlueprintType)
struct FMapInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FString MapDisplayName = "DisplayName"; // 显示的名字，比如 "沙漠废墟"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	FName LevelName = "LevelName";        // 真实的关卡名字，比如 "Level_Desert"

	// 在 C++ 里直接设置贴图，可以加上这个
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	UTexture2D* MapThumbnail; 
};

UCLASS()
class BATTLEBLASTER_API USelectMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 在蓝图中配置你们游戏所有的地图列表！
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Data")
	TArray<FMapInfo> AllMaps;
	// 记住是哪个 UI 打开了我
	UPROPERTY()
	UUserWidget* ParentUI = nullptr;
	// 记住上级让我用什么模式加载地图（用于动态切换模式）
	UPROPERTY()
	TSubclassOf<AGameModeBase> TargetGameModeClass;
protected:
	virtual void NativeConstruct() override;

	// ================= 绑定的 UI 控件 =================
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Map0;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Map1;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Map2;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Map3;

	UPROPERTY(meta = (BindWidget)) UBorder* Border_Map0;
	UPROPERTY(meta = (BindWidget)) UBorder* Border_Map1;
	UPROPERTY(meta = (BindWidget)) UBorder* Border_Map2;
	UPROPERTY(meta = (BindWidget)) UBorder* Border_Map3;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_MapName0;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_MapName1;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_MapName2;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_MapName3;

	UPROPERTY(meta = (BindWidget)) UButton* Btn_Confirm;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Back;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_PrevPage;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_NextPage;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_PageNumber;

	// ================= 状态数据 =================
	int32 CurrentPageIndex = 0;
	int32 SelectedGlobalMapIndex = -1; // -1 表示没选

	// ================= 函数 =================
	void UpdatePageDisplay();
	void HighlightSelectedMap();

	// 按钮事件
	UFUNCTION() void OnMap0Clicked();
	UFUNCTION() void OnMap1Clicked();
	UFUNCTION() void OnMap2Clicked();
	UFUNCTION() void OnMap3Clicked();

	UFUNCTION() void OnMapClicked(int32 ButtonIndex);

	UFUNCTION() void OnPrevPageClicked();
	UFUNCTION() void OnNextPageClicked();
	UFUNCTION() void OnConfirmClicked();
	UFUNCTION() void OnBackClicked();
};