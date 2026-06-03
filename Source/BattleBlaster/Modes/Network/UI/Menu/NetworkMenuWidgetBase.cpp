#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

UCanvasPanel* UNetworkMenuWidgetBase::BuildMenuRoot(const FString& Title, const FString& Subtitle, UVerticalBox*& OutContentBox)
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NetworkMenuRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetColorAndOpacity(FLinearColor(0.015f, 0.018f, 0.022f, 0.92f));
	if (UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	OutContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	if (UCanvasPanelSlot* ContentSlot = RootCanvas->AddChildToCanvas(OutContentBox))
	{
		ContentSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		ContentSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ContentSlot->SetPosition(FVector2D(0.0f, 0.0f));
		ContentSlot->SetSize(FVector2D(720.0f, 560.0f));
	}

	AddMenuText(OutContentBox, Title, 42, FLinearColor::White, 8.0f);
	if (!Subtitle.IsEmpty())
	{
		AddMenuText(OutContentBox, Subtitle, 18, FLinearColor(0.72f, 0.78f, 0.86f, 1.0f), 28.0f);
	}

	return RootCanvas;
}

UTextBlock* UNetworkMenuWidgetBase::AddMenuText(UVerticalBox* ContentBox, const FString& Text, int32 FontSize, const FLinearColor& Color, float BottomPadding)
{
	if (!ContentBox)
	{
		return nullptr;
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetJustification(ETextJustify::Center);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);

	if (UVerticalBoxSlot* TextSlot = ContentBox->AddChildToVerticalBox(TextBlock))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	return TextBlock;
}

UButton* UNetworkMenuWidgetBase::AddMenuButton(UVerticalBox* ContentBox, const FString& Label, float BottomPadding)
{
	if (!ContentBox)
	{
		return nullptr;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetJustification(ETextJustify::Center);

	FSlateFontInfo Font = LabelText->GetFont();
	Font.Size = 24;
	LabelText->SetFont(Font);
	Button->AddChild(LabelText);

	if (UVerticalBoxSlot* ButtonSlot = ContentBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	return Button;
}

UEditableTextBox* UNetworkMenuWidgetBase::AddEditableTextBox(UVerticalBox* ContentBox, const FString& Label, const FString& InitialText, float BottomPadding)
{
	if (!ContentBox)
	{
		return nullptr;
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 20;
	LabelText->SetFont(LabelFont);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetPadding(FMargin(0.0f, 8.0f, 16.0f, 0.0f));
	}

	UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	TextBox->SetText(FText::FromString(InitialText));
	if (UHorizontalBoxSlot* TextBoxSlot = Row->AddChildToHorizontalBox(TextBox))
	{
		TextBoxSlot->SetHorizontalAlignment(HAlign_Fill);
		TextBoxSlot->SetPadding(FMargin(0.0f));
	}

	return TextBox;
}

UTextBlock* UNetworkMenuWidgetBase::AddStepperRow(UVerticalBox* ContentBox, const FString& Label, UButton*& OutMinusButton, UButton*& OutPlusButton, float BottomPadding)
{
	OutMinusButton = nullptr;
	OutPlusButton = nullptr;
	if (!ContentBox)
	{
		return nullptr;
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	FSlateFontInfo LabelFont = LabelText->GetFont();
	LabelFont.Size = 20;
	LabelText->SetFont(LabelFont);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetPadding(FMargin(0.0f, 8.0f, 18.0f, 0.0f));
	}

	OutMinusButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* MinusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	MinusText->SetText(FText::FromString(TEXT("-")));
	MinusText->SetJustification(ETextJustify::Center);
	OutMinusButton->AddChild(MinusText);
	Row->AddChildToHorizontalBox(OutMinusButton);

	UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ValueText->SetJustification(ETextJustify::Center);
	FSlateFontInfo ValueFont = ValueText->GetFont();
	ValueFont.Size = 22;
	ValueText->SetFont(ValueFont);
	if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueText))
	{
		ValueSlot->SetPadding(FMargin(16.0f, 6.0f, 16.0f, 0.0f));
	}

	OutPlusButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* PlusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PlusText->SetText(FText::FromString(TEXT("+")));
	PlusText->SetJustification(ETextJustify::Center);
	OutPlusButton->AddChild(PlusText);
	Row->AddChildToHorizontalBox(OutPlusButton);

	return ValueText;
}

void UNetworkMenuWidgetBase::OpenChildMenu(UNetworkMenuWidgetBase* ChildMenu)
{
	if (!ChildMenu)
	{
		return;
	}

	ChildMenu->PreviousWidget = this;
	ChildMenu->ParentMenuWidget = ParentMenuWidget ? ParentMenuWidget : this;
	ChildMenu->AddToViewport(100);
	SetVisibility(ESlateVisibility::Hidden);
}

void UNetworkMenuWidgetBase::HandleBackClicked()
{
	RemoveFromParent();

	if (PreviousWidget)
	{
		PreviousWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else if (ParentMenuWidget)
	{
		ParentMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}
}
