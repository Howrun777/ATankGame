#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CppShowScoresWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UOverlay;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

USTRUCT(BlueprintType)
struct FNetworkScoreBarWidgets
{
	GENERATED_BODY()

	UPROPERTY()
	UBorder* RootBorder = nullptr;

	UPROPERTY()
	UProgressBar* ProgressBar = nullptr;

	UPROPERTY()
	UTextBlock* ScoreText = nullptr;

	UPROPERTY()
	UTextBlock* NameText = nullptr;
};

UCLASS()
class BATTLEBLASTER_API UCppShowScoresWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCppShowScoresWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void SetTargetScore(int32 InTargetScore);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void SetElapsedTime(int32 InElapsedSeconds);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void SetScores(const TArray<int32>& InScores);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void SetLocalPlayerSlotId(int32 InSlotId);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void UpdateScoreboard(int32 InTargetScore, int32 InElapsedSeconds, const TArray<int32>& InScores, int32 InLocalSlotId);

	UFUNCTION(BlueprintCallable, Category = "Network|Scores")
	void RefreshDisplay();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TargetAndTimeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UHorizontalBox* PlayerScoresBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor PanelColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor HeaderTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor OtherPlayerTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor OtherPlayerFillColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor LocalPlayerTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor LocalPlayerFillColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style")
	FLinearColor ScoreTrackColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style", meta = (ClampMin = "1.0"))
	float ScoreBarWidth = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style", meta = (ClampMin = "1.0"))
	float ScoreBarHeight = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style", meta = (ClampMin = "1.0"))
	int32 HeaderFontSize = 22;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Scores|Style", meta = (ClampMin = "1.0"))
	int32 ScoreFontSize = 18;

private:
	UPROPERTY()
	UCanvasPanel* GeneratedRootCanvas = nullptr;

	UPROPERTY()
	UBorder* GeneratedPanelBorder = nullptr;

	UPROPERTY()
	UVerticalBox* GeneratedPanelBox = nullptr;

	UPROPERTY()
	TArray<FNetworkScoreBarWidgets> GeneratedScoreBars;

	UPROPERTY()
	TArray<int32> CurrentScores;

	int32 TargetScore = 7;
	int32 ElapsedSeconds = 0;
	int32 LocalPlayerSlotId = INDEX_NONE;

	void BuildDefaultWidgetTree();
	bool HasBlueprintLayout() const;
	void RebuildGeneratedScoreBars();
	void BuildGeneratedScoreBar(int32 SlotId);
	void ApplyScoreBarState(int32 SlotId);
	FLinearColor GetFillColorForSlot(int32 SlotId, bool bIsLocalPlayer) const;
	FLinearColor GetPanelColorForSlot(int32 SlotId, bool bIsLocalPlayer) const;
	FString FormatElapsedTime() const;
	FText FormatHeaderText() const;
	FText FormatScoreText(int32 SlotId, int32 Score) const;
	static void ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification);
};
