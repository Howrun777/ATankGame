#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetworkMOBAStateWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

USTRUCT(BlueprintType)
struct FNetworkMOBATeamStatusWidgets
{
	GENERATED_BODY()

	UPROPERTY()
	UBorder* RootBorder = nullptr;

	UPROPERTY()
	UTextBlock* TeamText = nullptr;

	UPROPERTY()
	UTextBlock* CoreText = nullptr;

	UPROPERTY()
	UTextBlock* StateText = nullptr;
};

UCLASS()
class BATTLEBLASTER_API UNetworkMOBAStateWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UNetworkMOBAStateWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Network|MOBA")
	void UpdateMOBAState(int32 InElapsedSeconds, const TArray<int32>& InAliveCoreCountsByTeam, const TArray<bool>& InTeamEliminated, int32 InLocalTeamId, int32 InWinningTeamId);

	UFUNCTION(BlueprintCallable, Category = "Network|MOBA")
	void RefreshDisplay();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HeaderText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	UHorizontalBox* TeamStateBox = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor PanelColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor HeaderTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor AliveColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor DownColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor EliminatedColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style")
	FLinearColor LocalTeamColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style", meta = (ClampMin = "1.0"))
	float TeamCardWidth = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style", meta = (ClampMin = "1"))
	int32 HeaderFontSize = 22;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style", meta = (ClampMin = "1"))
	int32 TeamFontSize = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network|MOBA|Style", meta = (ClampMin = "1"))
	int32 StateFontSize = 18;

private:
	UPROPERTY()
	UCanvasPanel* GeneratedRootCanvas = nullptr;

	UPROPERTY()
	UBorder* GeneratedPanelBorder = nullptr;

	UPROPERTY()
	UVerticalBox* GeneratedPanelBox = nullptr;

	UPROPERTY()
	TArray<FNetworkMOBATeamStatusWidgets> GeneratedTeamStates;

	UPROPERTY()
	TArray<int32> AliveCoreCountsByTeam;

	UPROPERTY()
	TArray<bool> bTeamEliminated;

	int32 ElapsedSeconds = 0;
	int32 LocalTeamId = INDEX_NONE;
	int32 WinningTeamId = INDEX_NONE;

	void BuildDefaultWidgetTree();
	bool HasBlueprintLayout() const;
	void RebuildGeneratedTeamStates();
	void BuildGeneratedTeamState(int32 TeamId);
	void ApplyTeamState(int32 TeamId);
	FString FormatElapsedTime() const;
	FText FormatHeaderText() const;
	FText FormatCoreText(int32 TeamId) const;
	FText FormatStateText(int32 TeamId) const;
	FLinearColor GetTeamStateColor(int32 TeamId) const;
	FLinearColor GetTeamPanelColor(int32 TeamId) const;
	static void ApplyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification);
};
