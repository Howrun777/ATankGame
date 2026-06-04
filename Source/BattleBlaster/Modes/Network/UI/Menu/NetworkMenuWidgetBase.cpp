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
	constexpr float ContentWidth = 640.0f;
	constexpr float LabelWidth = 170.0f;
	constexpr float RowHeight = 52.0f;

	const FLinearColor MenuBackgroundColor(0.014f, 0.015f, 0.018f, 0.97f);
	const FLinearColor PanelColor(0.040f, 0.043f, 0.050f, 0.98f);
	const FLinearColor FieldColor(0.082f, 0.087f, 0.098f, 1.0f);
	const FLinearColor FieldHoverColor(0.105f, 0.112f, 0.126f, 1.0f);
	const FLinearColor AccentColor(0.11f, 0.64f, 0.58f, 1.0f);
	const FLinearColor AccentHoverColor(0.15f, 0.74f, 0.67f, 1.0f);
	const FLinearColor PressedColor(0.07f, 0.42f, 0.38f, 1.0f);
	const FLinearColor SecondaryButtonColor(0.118f, 0.124f, 0.140f, 1.0f);
	const FLinearColor SecondaryHoverColor(0.150f, 0.158f, 0.178f, 1.0f);
	const FLinearColor TextColor(0.92f, 0.94f, 0.96f, 1.0f);
	const FLinearColor MutedTextColor(0.62f, 0.66f, 0.70f, 1.0f);
	const FLinearColor NoticePanelColor(0.062f, 0.074f, 0.082f, 0.96f);
	const FLinearColor DividerColor(0.20f, 0.22f, 0.25f, 0.85f);

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
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(1.0f, 1.0f, -1.0f, -1.0f));

		Button->SetStyle(Style);
	}

	void ApplyCardButtonStyle(UButton* Button, bool bPrimary)
	{
		if (!Button)
		{
			return;
		}

		FButtonStyle Style = Button->GetStyle();
		FSlateBrush NormalBrush = Style.Normal;
		FSlateBrush HoveredBrush = Style.Hovered;
		FSlateBrush PressedBrush = Style.Pressed;

		NormalBrush.TintColor = FSlateColor(bPrimary ? FLinearColor(0.070f, 0.100f, 0.100f, 1.0f) : SecondaryButtonColor);
		HoveredBrush.TintColor = FSlateColor(bPrimary ? FLinearColor(0.085f, 0.130f, 0.126f, 1.0f) : SecondaryHoverColor);
		PressedBrush.TintColor = FSlateColor(bPrimary ? FLinearColor(0.052f, 0.080f, 0.080f, 1.0f) : FLinearColor(0.095f, 0.102f, 0.116f, 1.0f));

		Style.SetNormal(NormalBrush);
		Style.SetHovered(HoveredBrush);
		Style.SetPressed(PressedBrush);
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(1.0f, 1.0f, -1.0f, -1.0f));

		Button->SetStyle(Style);
	}

	void ApplyEditableTextBoxStyle(UEditableTextBox* TextBox)
	{
		if (!TextBox)
		{
			return;
		}

		FEditableTextBoxStyle Style = TextBox->WidgetStyle;
		FSlateBrush Background = Style.BackgroundImageNormal;
		FSlateBrush Hovered = Style.BackgroundImageHovered;
		FSlateBrush Focused = Style.BackgroundImageFocused;

		Background.TintColor = FSlateColor(FLinearColor(0.030f, 0.033f, 0.040f, 1.0f));
		Hovered.TintColor = FSlateColor(FieldHoverColor);
		Focused.TintColor = FSlateColor(FLinearColor(0.035f, 0.070f, 0.066f, 1.0f));

		Style.SetBackgroundImageNormal(Background);
		Style.SetBackgroundImageHovered(Hovered);
		Style.SetBackgroundImageFocused(Focused);
		TextBox->WidgetStyle = Style;
		TextBox->SetForegroundColor(TextColor);
		TextBox->SetHintText(FText::GetEmpty());
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
		TextBlock->SetLineHeightPercentage(1.0f);
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

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	PanelBorder->SetBrushColor(PanelColor);
	PanelBorder->SetPadding(FMargin(36.0f, 34.0f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBorder))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
		PanelSlot->SetSize(FVector2D(760.0f, 720.0f));
	}

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelBox"));
	PanelBorder->SetContent(PanelBox);

	UVerticalBox* HeaderBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HeaderBox"));
	if (UVerticalBoxSlot* HeaderSlot = PanelBox->AddChildToVerticalBox(HeaderBox))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UTextBlock* EyebrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EyebrowText"));
	EyebrowText->SetText(FText::FromString(TEXT("NETWORK")));
	StyleText(EyebrowText, 13, AccentColor, ETextJustify::Left);
	if (UVerticalBoxSlot* EyebrowSlot = HeaderBox->AddChildToVerticalBox(EyebrowText))
	{
		EyebrowSlot->SetHorizontalAlignment(HAlign_Fill);
		EyebrowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(Title));
	StyleText(TitleText, 32, TextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* TitleSlot = HeaderBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Subtitle.IsEmpty() ? 0.0f : 6.0f));
	}

	if (!Subtitle.IsEmpty())
	{
		UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		SubtitleText->SetText(FText::FromString(Subtitle));
		SubtitleText->SetAutoWrapText(true);
		StyleText(SubtitleText, 15, MutedTextColor, ETextJustify::Left);
		if (UVerticalBoxSlot* SubtitleSlot = HeaderBox->AddChildToVerticalBox(SubtitleText))
		{
			SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
			SubtitleSlot->SetPadding(FMargin(0.0f));
		}
	}

	USizeBox* SeparatorBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HeaderSeparatorBox"));
	SeparatorBox->SetHeightOverride(1.0f);
	UImage* Separator = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HeaderSeparator"));
	Separator->SetColorAndOpacity(DividerColor);
	SeparatorBox->AddChild(Separator);
	if (UVerticalBoxSlot* SeparatorSlot = PanelBox->AddChildToVerticalBox(SeparatorBox))
	{
		SeparatorSlot->SetHorizontalAlignment(HAlign_Fill);
		SeparatorSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
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
	ButtonBox->SetWidthOverride(ContentWidth);
	ButtonBox->SetHeightOverride(RowHeight);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyButtonStyle(Button, !Label.Contains(TEXT("Back")) && !Label.Contains(TEXT("Not Available")));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	StyleText(LabelText, 18, TextColor, ETextJustify::Center);
	Button->AddChild(LabelText);
	ButtonBox->AddChild(Button);

	if (UVerticalBoxSlot* ButtonSlot = ContentBox->AddChildToVerticalBox(ButtonBox))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	return Button;
}

UButton* UNetworkMenuWidgetBase::AddMenuCardButton(UVerticalBox* ContentBox, const FString& Title, const FString& Subtitle, const FString& Tag, bool bPrimary, float BottomPadding)
{
	if (!ContentBox)
	{
		return nullptr;
	}

	USizeBox* CardBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	CardBox->SetWidthOverride(ContentWidth);
	CardBox->SetHeightOverride(92.0f);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	ApplyCardButtonStyle(Button, bPrimary);

	UHorizontalBox* CardContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	UVerticalBox* TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (UHorizontalBoxSlot* TextColumnSlot = CardContent->AddChildToHorizontalBox(TextColumn))
	{
		TextColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		TextColumnSlot->SetVerticalAlignment(VAlign_Center);
		TextColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextColumnSlot->SetPadding(FMargin(18.0f, 0.0f, 14.0f, 0.0f));
	}

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TitleText->SetText(FText::FromString(Title));
	StyleText(TitleText, 20, TextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* TitleSlot = TextColumn->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
	}

	UTextBlock* SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	SubtitleText->SetText(FText::FromString(Subtitle));
	SubtitleText->SetAutoWrapText(true);
	StyleText(SubtitleText, 14, MutedTextColor, ETextJustify::Left);
	if (UVerticalBoxSlot* SubtitleSlot = TextColumn->AddChildToVerticalBox(SubtitleText))
	{
		SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	if (!Tag.IsEmpty())
	{
		UBorder* TagBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		TagBorder->SetBrushColor(bPrimary ? FLinearColor(0.060f, 0.420f, 0.380f, 1.0f) : FLinearColor(0.165f, 0.172f, 0.188f, 1.0f));
		TagBorder->SetPadding(FMargin(10.0f, 4.0f));

		UTextBlock* TagText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TagText->SetText(FText::FromString(Tag));
		StyleText(TagText, 12, bPrimary ? TextColor : MutedTextColor, ETextJustify::Center);
		TagBorder->SetContent(TagText);

		USizeBox* TagBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		TagBox->SetWidthOverride(88.0f);
		TagBox->AddChild(TagBorder);
		if (UHorizontalBoxSlot* TagSlot = CardContent->AddChildToHorizontalBox(TagBox))
		{
			TagSlot->SetHorizontalAlignment(HAlign_Right);
			TagSlot->SetVerticalAlignment(VAlign_Center);
			TagSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		}
	}

	Button->AddChild(CardContent);
	CardBox->AddChild(Button);

	if (UVerticalBoxSlot* CardSlot = ContentBox->AddChildToVerticalBox(CardBox))
	{
		CardSlot->SetHorizontalAlignment(HAlign_Center);
		CardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
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
	RowBox->SetWidthOverride(ContentWidth);
	RowBox->SetHeightOverride(RowHeight);
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
	StyleText(LabelText, 16, MutedTextColor, ETextJustify::Left);
	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelBox->SetWidthOverride(LabelWidth);
	LabelBox->AddChild(LabelText);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	}

	UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	TextBox->SetText(FText::FromString(InitialText));
	ApplyEditableTextBoxStyle(TextBox);
	USizeBox* TextBoxSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TextBoxSize->SetWidthOverride(420.0f);
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
	RowBox->SetWidthOverride(ContentWidth);
	RowBox->SetHeightOverride(RowHeight);
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
	StyleText(LabelText, 16, MutedTextColor, ETextJustify::Left);
	USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LabelBox->SetWidthOverride(LabelWidth);
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
	MinusBox->SetHeightOverride(38.0f);
	MinusBox->AddChild(OutMinusButton);
	Row->AddChildToHorizontalBox(MinusBox);

	UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StyleText(ValueText, 18, TextColor, ETextJustify::Center);
	USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ValueBox->SetWidthOverride(300.0f);
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
	PlusBox->SetHeightOverride(38.0f);
	PlusBox->AddChild(OutPlusButton);
	Row->AddChildToHorizontalBox(PlusBox);

	return ValueText;
}

UTextBlock* UNetworkMenuWidgetBase::AddNoticeText(UVerticalBox* ContentBox, const FString& Text, const FLinearColor& InAccentColor, float BottomPadding)
{
	if (!ContentBox)
	{
		return nullptr;
	}

	USizeBox* NoticeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	NoticeBox->SetWidthOverride(ContentWidth);
	NoticeBox->SetMinDesiredHeight(54.0f);

	UBorder* NoticeBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	NoticeBorder->SetBrushColor(NoticePanelColor);
	NoticeBorder->SetPadding(FMargin(14.0f, 10.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	NoticeBorder->SetContent(Row);

	USizeBox* AccentBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	AccentBox->SetWidthOverride(4.0f);
	UImage* Accent = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Accent->SetColorAndOpacity(InAccentColor);
	AccentBox->AddChild(Accent);
	if (UHorizontalBoxSlot* AccentSlot = Row->AddChildToHorizontalBox(AccentBox))
	{
		AccentSlot->SetVerticalAlignment(VAlign_Fill);
		AccentSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetAutoWrapText(true);
	StyleText(TextBlock, 16, TextColor, ETextJustify::Left);
	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextBlock))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	NoticeBox->AddChild(NoticeBorder);
	if (UVerticalBoxSlot* NoticeSlot = ContentBox->AddChildToVerticalBox(NoticeBox))
	{
		NoticeSlot->SetHorizontalAlignment(HAlign_Center);
		NoticeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}

	return TextBlock;
}

void UNetworkMenuWidgetBase::AddSectionHeader(UVerticalBox* ContentBox, const FString& Label, float BottomPadding)
{
	if (!ContentBox)
	{
		return;
	}

	UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HeaderText->SetText(FText::FromString(Label));
	StyleText(HeaderText, 13, AccentColor, ETextJustify::Left);

	USizeBox* HeaderBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	HeaderBox->SetWidthOverride(ContentWidth);
	HeaderBox->AddChild(HeaderText);

	if (UVerticalBoxSlot* HeaderSlot = ContentBox->AddChildToVerticalBox(HeaderBox))
	{
		HeaderSlot->SetHorizontalAlignment(HAlign_Center);
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
	}
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
