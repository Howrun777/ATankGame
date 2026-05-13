#include "SelectMapWidget.h"






#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"
#include "Components/Border.h"

void USelectMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定点击事件
	if (Btn_Map0) Btn_Map0->OnClicked.AddDynamic(this, &USelectMapWidget::OnMap0Clicked);
	if (Btn_Map1) Btn_Map1->OnClicked.AddDynamic(this, &USelectMapWidget::OnMap1Clicked);
	if (Btn_Map2) Btn_Map2->OnClicked.AddDynamic(this, &USelectMapWidget::OnMap2Clicked);
	if (Btn_Map3) Btn_Map3->OnClicked.AddDynamic(this, &USelectMapWidget::OnMap3Clicked);

	if (Btn_PrevPage) Btn_PrevPage->OnClicked.AddDynamic(this, &USelectMapWidget::OnPrevPageClicked);
	if (Btn_NextPage) Btn_NextPage->OnClicked.AddDynamic(this, &USelectMapWidget::OnNextPageClicked);
	if (Btn_Confirm) Btn_Confirm->OnClicked.AddDynamic(this, &USelectMapWidget::OnConfirmClicked);
	if (Btn_Back) Btn_Back->OnClicked.AddDynamic(this, &USelectMapWidget::OnBackClicked);

	// 初始化显示
	CurrentPageIndex = 0;
	SelectedGlobalMapIndex = -1;
	UpdatePageDisplay();
}

void USelectMapWidget::UpdatePageDisplay()
{
	UButton* MapButtons[4] = { Btn_Map0, Btn_Map1, Btn_Map2, Btn_Map3 };
	UTextBlock* MapTexts[4] = { Text_MapName0, Text_MapName1, Text_MapName2, Text_MapName3 };

	int32 StartIndex = CurrentPageIndex * 4;

	for (int32 i = 0; i < 4; i++)
	{
		int32 GlobalIndex = StartIndex + i;

		if (AllMaps.IsValidIndex(GlobalIndex))
		{
			// 1. 显示按钮和文字
			MapButtons[i]->SetVisibility(ESlateVisibility::Visible);
			MapTexts[i]->SetVisibility(ESlateVisibility::Visible);
			MapTexts[i]->SetText(FText::FromString(AllMaps[GlobalIndex].MapDisplayName));

			// 2. 【最稳妥的贴图替换法】
			if (AllMaps[GlobalIndex].MapThumbnail)
			{
				FButtonStyle NewStyle = MapButtons[i]->GetStyle();

				// 创建一个新的画刷 (Brush)
				FSlateBrush NewBrush;
				NewBrush.SetResourceObject(AllMaps[GlobalIndex].MapThumbnail);

				// 【关键1】：告诉引擎，这张图要作为 Image 渲染（铺满整个控件）
				NewBrush.DrawAs = ESlateBrushDrawType::Image;
				// 【关键2】：给一个默认的基础尺寸防崩溃
				NewBrush.ImageSize = FVector2D(256.f, 256.f);

				// 把三个状态都替换成这个新画刷
				NewStyle.Normal = NewBrush;
				NewStyle.Hovered = NewBrush;
				NewStyle.Pressed = NewBrush;

				// 加上互动变暗效果
				NewStyle.Hovered.TintColor = FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.0f));
				NewStyle.Pressed.TintColor = FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));

				// 重新穿回给按钮
				MapButtons[i]->SetStyle(NewStyle);
			}
		}
		else
		{
			// 如果没地图，彻底隐藏！
			MapButtons[i]->SetVisibility(ESlateVisibility::Hidden);
			MapTexts[i]->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 更新页码文字
	int32 TotalPages = FMath::CeilToInt((float)AllMaps.Num() / 4.0f);
	if (TotalPages == 0) TotalPages = 1;
	if (Text_PageNumber)
	{
		Text_PageNumber->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentPageIndex + 1, TotalPages)));
	}

	HighlightSelectedMap();
}

// 封装点击逻辑
void USelectMapWidget::OnMap0Clicked() { OnMapClicked(0); }
void USelectMapWidget::OnMap1Clicked() { OnMapClicked(1); }
void USelectMapWidget::OnMap2Clicked() { OnMapClicked(2); }
void USelectMapWidget::OnMap3Clicked() { OnMapClicked(3); }

void USelectMapWidget::OnMapClicked(int32 ButtonIndex)
{
	// 计算全局索引
	int32 GlobalIndex = (CurrentPageIndex * 4) + ButtonIndex;

	if (AllMaps.IsValidIndex(GlobalIndex))
	{
		SelectedGlobalMapIndex = GlobalIndex;
		HighlightSelectedMap();
	}
}

void USelectMapWidget::HighlightSelectedMap()
{
	// 拿到所有的按钮和相框
	UButton* MapButtons[4] = { Btn_Map0, Btn_Map1, Btn_Map2, Btn_Map3 };
	UBorder* MapBorders[4] = { Border_Map0, Border_Map1, Border_Map2, Border_Map3 };

	for (int32 i = 0; i < 4; i++)
	{
		int32 GlobalIndex = (CurrentPageIndex * 4) + i;

		// 【修复】：先把按钮本身的背景色重置为白色(无滤镜状态)，防止之前残留的绿色滤镜
		MapButtons[i]->SetBackgroundColor(FLinearColor::White);

		if (GlobalIndex == SelectedGlobalMapIndex && SelectedGlobalMapIndex != -1)
		{
			// 【选中状态】：相框变成亮绿色，且不透明 (Alpha = 1.0)
			// 你可以调 FLinearColor(R, G, B, A) 的数值来换成你喜欢的颜色，比如金色、蓝色
			MapBorders[i]->SetBrushColor(FLinearColor(1.0f, 1.0f, 0.0f, 1.0f));
			//MapBorders[i]->SetBrushColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));

		}
		else
		{
			// 【未选中状态】：相框变成完全透明 (Alpha = 0.0)
			MapBorders[i]->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
		}
	}
}
void USelectMapWidget::OnPrevPageClicked()
{
	if (CurrentPageIndex > 0)
	{
		CurrentPageIndex--;
		UpdatePageDisplay();
	}
}

void USelectMapWidget::OnNextPageClicked()
{
	int32 TotalPages = FMath::CeilToInt((float)AllMaps.Num() / 4.0f);
	if (CurrentPageIndex < TotalPages - 1)
	{
		CurrentPageIndex++;
		UpdatePageDisplay();
	}
}

void USelectMapWidget::OnConfirmClicked()
{
	if (SelectedGlobalMapIndex != -1 && AllMaps.IsValidIndex(SelectedGlobalMapIndex))
	{
		FName LevelToLoad = AllMaps[SelectedGlobalMapIndex].LevelName;

		// 拼接虚幻引擎专属的 URL 选项字符串
		FString OptionsString = TEXT("");

		if (TargetGameModeClass)
		{
			// 获取我们传入的 GameMode 的完整路径，格式类似："?game=/Game/BP_MyMode.BP_MyMode_C"
			FString ClassPath = TargetGameModeClass->GetPathName();
			OptionsString = FString::Printf(TEXT("?game=%s"), *ClassPath);
		}

		UE_LOG(LogTemp, Warning, TEXT("准备加载地图: %s, 附加参数: %s"), *LevelToLoad.ToString(), *OptionsString);

		// 【关键】：带有 Options 参数的 OpenLevel 方式
		// 参数3 (bAbsolute) 设为 true，代表无视原有设置，绝对应用我们的新选项
		UGameplayStatics::OpenLevel(GetWorld(), LevelToLoad, true, OptionsString);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("请先选中一个地图！"));
	}
}

// 顺便把返回逻辑也写上
void USelectMapWidget::OnBackClicked()
{
	// 恢复显示上一级UI
	if (ParentUI)
	{
		ParentUI->SetVisibility(ESlateVisibility::Visible);
	}
	// 销毁地图UI自己
	RemoveFromParent();
}