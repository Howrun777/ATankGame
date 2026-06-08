#pragma once

#include "CoreMinimal.h"
#include "Core/Networking/BattleBlasterNetworkTypes.h"
#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"
#include "NetworkMapSelectWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetworkMapSelectedSignature, const FNetworkMapInfo&, SelectedMap);

class UBorder;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;

UCLASS()
class BATTLEBLASTER_API UNetworkMapSelectWidget : public UNetworkMenuWidgetBase
{
	GENERATED_BODY()

public:
	UNetworkMapSelectWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "Network|Map Select")
	FNetworkMapSelectedSignature OnNetworkMapSelected;

	UFUNCTION(BlueprintCallable, Category = "Network|Map Select")
	void SetupForMode(ENetworkGameModeType InModeType, int32 InPlayerCount, const FNetworkMapInfo& InCurrentMap);

	UFUNCTION(BlueprintPure, Category = "Network|Map Select")
	const TArray<FNetworkMapInfo>& GetMapsForCurrentMode() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Maps")
	TArray<FNetworkMapInfo> DeathmatchMaps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Maps")
	TArray<FNetworkMapInfo> TeamDeathmatchMaps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Maps")
	TArray<FNetworkMapInfo> MOBAMaps;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Maps")
	TArray<FNetworkMapInfo> TeamMOBAMaps;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Map0 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Map1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Map2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Map3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* Border_Map0 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* Border_Map1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* Border_Map2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* Border_Map3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_MapName0 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_MapName1 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_MapName2 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_MapName3 = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Confirm = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_Back = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_PrevPage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_NextPage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_PageNumber = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_Status = nullptr;

private:
	UPROPERTY()
	UCanvasPanel* GeneratedRootCanvas = nullptr;

	UPROPERTY()
	TArray<UImage*> GeneratedMapImages;

	UPROPERTY()
	TArray<FNetworkMapInfo> EmptyMaps;

	ENetworkGameModeType CurrentModeType = ENetworkGameModeType::Deathmatch;
	int32 CurrentPlayerCount = 2;
	int32 CurrentPageIndex = 0;
	int32 SelectedGlobalMapIndex = INDEX_NONE;

	void BuildDefaultWidgetTree();
	void BindButtons();
	void EnsureDefaultMaps();
	void UpdatePageDisplay();
	void HighlightSelectedMap();
	void SelectMapAtButtonIndex(int32 ButtonIndex);
	void ConfirmSelectedMap();
	void ApplyMapButtonVisual(int32 ButtonIndex, const FNetworkMapInfo& MapInfo);
	void SetMapButtonVisible(int32 ButtonIndex, bool bVisible);
	bool IsMapAllowedForPlayerCount(const FNetworkMapInfo& MapInfo) const;
	int32 FindMapIndexByLevelName(const FName& LevelName) const;
	FText BuildMapSubtitle(const FNetworkMapInfo& MapInfo) const;
	static FName ResolveLevelName(const FNetworkMapInfo& MapInfo);
	static void ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification);

	UFUNCTION()
	void OnMap0Clicked();

	UFUNCTION()
	void OnMap1Clicked();

	UFUNCTION()
	void OnMap2Clicked();

	UFUNCTION()
	void OnMap3Clicked();

	UFUNCTION()
	void OnPrevPageClicked();

	UFUNCTION()
	void OnNextPageClicked();

	UFUNCTION()
	void OnConfirmClicked();
};
