#include "Modes/Network/UI/NetworkJoinMessageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

UNetworkJoinMessageWidget::UNetworkJoinMessageWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MessageColor = FLinearColor::Black;
	FontSize = 32;
	DisplayDuration = 3.0f;
	ScreenOffset = FVector2D(48.0f, 0.0f);
}

TSharedRef<SWidget> UNetworkJoinMessageWidget::RebuildWidget()
{
	BuildDefaultWidgetTree();
	return Super::RebuildWidget();
}

void UNetworkJoinMessageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyMessageStyle();
}

void UNetworkJoinMessageWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}

	Super::NativeDestruct();
}

void UNetworkJoinMessageWidget::ShowMessage(const FString& Message)
{
	CurrentMessage = Message;

	if (MessageTextBlock)
	{
		MessageTextBlock->SetText(FText::FromString(CurrentMessage));
		ApplyMessageStyle();
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().SetTimer(
			HideTimerHandle,
			this,
			&UNetworkJoinMessageWidget::HideMessage,
			DisplayDuration,
			false);
	}
}

void UNetworkJoinMessageWidget::HideMessage()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UNetworkJoinMessageWidget::BuildDefaultWidgetTree()
{
	if (HasBlueprintLayout() || GeneratedRootCanvas)
	{
		return;
	}

	GeneratedRootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NetworkJoinMessageRoot"));
	WidgetTree->RootWidget = GeneratedRootCanvas;

	MessageTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageTextBlock"));
	MessageTextBlock->SetText(FText::FromString(CurrentMessage));
	ApplyMessageStyle();

	if (UCanvasPanelSlot* MessageSlot = GeneratedRootCanvas->AddChildToCanvas(MessageTextBlock))
	{
		MessageSlot->SetAnchors(FAnchors(0.0f, 0.5f));
		MessageSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		MessageSlot->SetPosition(ScreenOffset);
		MessageSlot->SetAutoSize(true);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

bool UNetworkJoinMessageWidget::HasBlueprintLayout() const
{
	return GeneratedRootCanvas == nullptr && WidgetTree && WidgetTree->RootWidget != nullptr;
}

void UNetworkJoinMessageWidget::ApplyMessageStyle()
{
	if (!MessageTextBlock)
	{
		return;
	}

	FSlateFontInfo Font = MessageTextBlock->GetFont();
	Font.Size = FontSize;
	MessageTextBlock->SetFont(Font);
	MessageTextBlock->SetColorAndOpacity(FSlateColor(MessageColor));
	MessageTextBlock->SetJustification(ETextJustify::Left);
	MessageTextBlock->SetShadowOffset(FVector2D(1.5f, 1.5f));
	MessageTextBlock->SetShadowColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.35f));
}
