#include "Modes/Network/UI/Menu/NetworkMenuWidgetBase.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	const FLinearColor MenuBackgroundColor(0.012f, 0.013f, 0.015f, 0.96f);
	const FLinearColor PanelColor(0.055f, 0.058f, 0.064f, 0.97f);
	const FLinearColor FieldColor(0.095f, 0.10f, 0.108f, 0.98f);
	const FLinearColor AccentColor(0.12f, 0.72f, 0.66f, 1.0f);
	const FLinearColor AccentHoverColor(0.18f, 0.86f, 0.78f, 1.0f);
	const FLinearColor PressedColor(0.08f, 0.46f, 0.44f, 1.0f);
	const FLinearColor SecondaryButtonColor(0.15f, 0.155f, 0.165f, 1.0f);
	const FLinearColor SecondaryHoverColor(0.22f, 0.225f, 0.24f, 1.0f);
	const FLinearColor TextColor(0.92f, 0.94f, 0.96f, 1.0f);
	const FLinearColor MutedTextColor(0.66f, 0.70f, 0.74f, 1.0f);

	void ApplyButtonStyle(UButton* Button, bool bPrimary)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		FSlateBrush NormalBrush = Style.Normal;
		FSlateBrush HoveredBrush = Style.Hovered;
		FSlateBrush PressedBrush = Style.Pressed;

		NormalBrush.TintColor = FSlateColor(bPrimary ? AccentColor : SecondaryButtonColor);
		HoveredBrush.TintColor = FSlateColor(bPrimary ? AccentHoverColor : SecondaryHoverColor);
		PressedBrush.TintColor = FSlateColor(PressedColor);

		Style.SetNormal(NormalBrush);
		Style.SetHovered(HoveredBrush);
		Style.SetPressed(PressedBrush);
		Style.SetNormalPadding(FMargin(14.0f, 8.0f));
		Style.SetPressedPadding(FMargin(15.0f, 9.0f, 13.0f, 7.0f));

		Button->SetStyle(Style);
	}

	void StyleText(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color, ETextJustify::Type Justification = ETextJustify::Left)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(Justification);
	}
}

UCanvasPanel* UNetworkMenuWidgetBase::BuildMenuRoot(const FString& Title, const FString& Subtitle, UVerticalBox*& OutContentBox)
{
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("NetworkMenuRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	Background->SetColorAndOpacity(MenuBackgroundColor);
	if (UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(Background))
	{
		BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackgroundSlot->SetOffsets(FMargin(0.0f));
	}

	UImage* AccentBand = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("AccentBand"));
	AccentBand->SetColorAndOpacity(FLinearColor(0.12f, 0.72f, 0.66f, 0.18f));
	if (UCanvasPanelSlot* AccentSlot = RootCanvas->AddChildToCanvas(AccentBand))
	{
		AccentSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 1.0f));
		AccentSlot->SetOffsets(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	PanelBorder->SetBrushColor(PanelColor);
	PanelBorder->SetPadding(FMargin(34.0f, 30.0f, 34.0f, 30.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
		PanelSlot->SetSize(FVector2D(900.0f, 760.0f));
	}

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	PanelBorder->SetContent(PanelBox);

	UTextBlock* EyebrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EyebrowText"));
	EyebrowText->SetText(FText::FromString(TEXT("NETWORK")));
	StyleText(EyebrowText, 15, AccentColor, ETextJustify::Left);
	if (UVerticalBoxSlot* EyebrowSlot = PanelBox->AddChildToVerticalBox(EyebrowText))
	{
		EyebrowSlot->SetHorizontalAlignment(HAlign_Fill);
		EyebrowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(Title));
	StyleText(TitleText, 40, TextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* TitleSlot = PanelBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Subtitle.IsEmpty() ? 20.0f : 6.0f));
	}

	if (!Subtitle.IsEmpty())
	{
		UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		SubtitleText->SetText(FText::FromString(Subtitle));
		SubtitleText->SetAutoWrapText(true);
		StyleText(SubtitleText, 17, MutedTextColor, ETextJustify::Left);
		if (UVerticalBoxSlot* SubtitleSlot = PanelBox->AddChildToVerticalBox(SubtitleText))
		{
			SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
			SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}
	}

	USizeBox* SeparatorBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HeaderSeparatorBox"));
	SeparatorBox->SetHeightOverride(2.0f);
	UImage* Separator = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HeaderSeparator"));
	Separator->SetColorAndOpacity(FLinearColor(0.12f, 0.72f, 0.66f, 0.7f));
	SeparatorBox->AddChild(Separator);
	if (UVerticalBoxSlot* SeparatorSlot = PanelBox->AddChildToVerticalBox(SeparatorBox))
	{
		SeparatorSlot->SetHorizontalAlignment(HAlign_Fill);
		SeparatorSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
	}

	OutContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	if (UVerticalBoxSlot* ContentSlot = PanelBox->AddChildToVerticalBox(OutContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Top);
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
	TextBlock->SetAutoWrapText(true);
	StyleText(TextBlock, FontSize, Color, ETextJustify::Left);

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

	USizeBox* ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonBox->SetWidthOverride(460.0f);
	ButtonBox->SetHeightOverride(58.0f);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyButtonStyle(Button, !Label.Contains(TEXT("Back")) && !Label.Contains(TEXT("Not Available")));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	StyleText(LabelText, 22, TextColor, ETextJustify::Center);
	Button->AddChild(LabelText);
	ButtonBox->AddChild(Button);

	if (UVerticalBoxSlot* ButtonSlot = ContentBox->AddChildToVerticalBox(ButtonBox))
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

	USizeBox* RowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RowBox->SetWidthOverride(600.0f);
	RowBox->SetHeightOverride(54.0f);
	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowBorder->SetBrushColor(FieldColor);
	RowBorder->SetPadding(FMargin(16.0f, 6.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowBorder->SetContent(Row);
	RowBox->AddChild(RowBorder);

	if (UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(RowBox))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	StyleText(LabelText, 18, MutedTextColor, ETextJustify::Left);
	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelBox->SetWidthOverride(150.0f);
	LabelBox->AddChild(LabelText);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	TextBox->SetText(FText::FromString(InitialText));
	USizeBox* TextBoxSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TextBoxSize->SetWidthOverride(392.0f);
	TextBoxSize->AddChild(TextBox);
	if (UHorizontalBoxSlot* TextBoxSlot = Row->AddChildToHorizontalBox(TextBoxSize))
	{
		TextBoxSlot->SetHorizontalAlignment(HAlign_Fill);
		TextBoxSlot->SetVerticalAlignment(VAlign_Center);
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

	USizeBox* RowBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	RowBox->SetWidthOverride(600.0f);
	RowBox->SetHeightOverride(54.0f);
	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowBorder->SetBrushColor(FieldColor);
	RowBorder->SetPadding(FMargin(16.0f, 6.0f));
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	RowBorder->SetContent(Row);
	RowBox->AddChild(RowBorder);

	if (UVerticalBoxSlot* RowSlot = ContentBox->AddChildToVerticalBox(RowBox))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	StyleText(LabelText, 18, MutedTextColor, ETextJustify::Left);
	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelBox->SetWidthOverride(220.0f);
	LabelBox->AddChild(LabelText);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	OutMinusButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyButtonStyle(OutMinusButton, false);
	UTextBlock* MinusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	MinusText->SetText(FText::FromString(TEXT("-")));
	StyleText(MinusText, 22, TextColor, ETextJustify::Center);
	OutMinusButton->AddChild(MinusText);
	USizeBox* MinusBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	MinusBox->SetWidthOverride(46.0f);
	MinusBox->SetHeightOverride(40.0f);
	MinusBox->AddChild(OutMinusButton);
	Row->AddChildToHorizontalBox(MinusBox);

	UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StyleText(ValueText, 21, TextColor, ETextJustify::Center);
	USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValueBox->SetWidthOverride(168.0f);
	ValueBox->AddChild(ValueText);
	if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueBox))
	{
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		ValueSlot->SetPadding(FMargin(14.0f, 0.0f));
	}

	OutPlusButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyButtonStyle(OutPlusButton, false);
	UTextBlock* PlusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	PlusText->SetText(FText::FromString(TEXT("+")));
	StyleText(PlusText, 22, TextColor, ETextJustify::Center);
	OutPlusButton->AddChild(PlusText);
	USizeBox* PlusBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	PlusBox->SetWidthOverride(46.0f);
	PlusBox->SetHeightOverride(40.0f);
	PlusBox->AddChild(OutPlusButton);
	Row->AddChildToHorizontalBox(PlusBox);

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
