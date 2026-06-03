#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetworkMenuWidgetBase.generated.h"

class UButton;
class UCanvasPanel;
class UEditableTextBox;
class UTextBlock;
class UUserWidget;
class UVerticalBox;

UCLASS()
class BATTLEBLASTER_API UNetworkMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Network Menu")
	UUserWidget* PreviousWidget = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Network Menu")
	UUserWidget* ParentMenuWidget = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Network Menu")
	void SetPreviousWidget(UUserWidget* InPreviousWidget) { PreviousWidget = InPreviousWidget; }

	UFUNCTION(BlueprintCallable, Category = "Network Menu")
	void SetParentMenuWidget(UUserWidget* InParentMenuWidget) { ParentMenuWidget = InParentMenuWidget; }

protected:
	UCanvasPanel* BuildMenuRoot(const FString& Title, const FString& Subtitle, UVerticalBox*& OutContentBox);
	UTextBlock* AddMenuText(UVerticalBox* ContentBox, const FString& Text, int32 FontSize, const FLinearColor& Color, float BottomPadding = 12.0f);
	UButton* AddMenuButton(UVerticalBox* ContentBox, const FString& Label, float BottomPadding = 12.0f);
	UEditableTextBox* AddEditableTextBox(UVerticalBox* ContentBox, const FString& Label, const FString& InitialText, float BottomPadding = 12.0f);
	UTextBlock* AddStepperRow(UVerticalBox* ContentBox, const FString& Label, UButton*& OutMinusButton, UButton*& OutPlusButton, float BottomPadding = 12.0f);

	void OpenChildMenu(UNetworkMenuWidgetBase* ChildMenu);

	UFUNCTION()
	virtual void HandleBackClicked();
};
