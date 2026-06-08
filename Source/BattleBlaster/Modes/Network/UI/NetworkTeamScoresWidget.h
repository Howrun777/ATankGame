#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetworkTeamScoresWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UOverlay;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

USTRUCT(BlueprintType)
struct FNetworkTeamScoreBarWidgets
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
class BATTLEBLASTER_API UNetworkTeamScoresWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNetworkTeamScoresWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Network|Team Scores")
	void UpdateTeamScoreboard(int32 InTargetScore, int32 InElapsedSeconds, const TArray<int32>& InTeamScores, int32 InLocalTeamId, int32 InWinningTeamId);

	UFUNCTION(BlueprintCallable, Category = "Network|Team Scores")
	void RefreshDisplay();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* TargetAndTimeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UHorizontalBox* TeamScoresBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor PanelColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor HeaderTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor OtherTeamTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor OtherTeamFillColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor LocalTeamTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor LocalTeamFillColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style")
	FLinearColor ScoreTrackColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style", meta = (ClampMin = "1.0"))
	float ScoreBarWidth = 170.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style", meta = (ClampMin = "1.0"))
	float ScoreBarHeight = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style", meta = (ClampMin = "1"))
	int32 HeaderFontSize = 22;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|Team Scores|Style", meta = (ClampMin = "1"))
	int32 ScoreFontSize = 18;

private:
	UPROPERTY()
	UCanvasPanel* GeneratedRootCanvas = nullptr;

	UPROPERTY()
	UBorder* GeneratedPanelBorder = nullptr;

	UPROPERTY()
	UVerticalBox* GeneratedPanelBox = nullptr;

	UPROPERTY()
	TArray<FNetworkTeamScoreBarWidgets> GeneratedTeamBars;

	UPROPERTY()
	TArray<int32> CurrentTeamScores;

	int32 TargetScore = 7;
	int32 ElapsedSeconds = 0;
	int32 LocalTeamId = INDEX_NONE;
	int32 WinningTeamId = INDEX_NONE;

	void BuildDefaultWidgetTree();
	bool HasBlueprintLayout() const;
	void RebuildGeneratedTeamBars();
	void BuildGeneratedTeamBar(int32 TeamId);
	void ApplyTeamBarState(int32 TeamId);
	FLinearColor GetFillColorForTeam(int32 TeamId, bool bIsLocalTeam) const;
	FLinearColor GetPanelColorForTeam(int32 TeamId, bool bIsLocalTeam) const;
	FString FormatElapsedTime() const;
	FText FormatHeaderText() const;
	FText FormatScoreText(int32 TeamId, int32 Score) const;
	static void ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification);
};
