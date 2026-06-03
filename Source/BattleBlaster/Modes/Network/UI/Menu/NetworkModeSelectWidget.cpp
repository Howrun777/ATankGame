#include "Modes/Network/UI/Menu/NetworkModeSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Modes/Network/UI/Menu/LANMenuWidget.h"

TSharedRef<SWidget> UNetworkModeSelectWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Network Game"), TEXT("Choose the network entry type."), ContentBox);

	LANButton = AddMenuButton(ContentBox, TEXT("LAN Game"));
	ServerButton = AddMenuButton(ContentBox, TEXT("Server Game (Not Available)"));
	StatusText = AddMenuText(ContentBox, TEXT(""), 18, FLinearColor(0.9f, 0.72f, 0.35f, 1.0f), 20.0f);
	BackButton = AddMenuButton(ContentBox, TEXT("Back"));

	return Super::RebuildWidget();
}

void UNetworkModeSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LANButton)
	{
		LANButton->OnClicked.AddDynamic(this, &UNetworkModeSelectWidget::HandleLANClicked);
	}
	if (ServerButton)
	{
		ServerButton->OnClicked.AddDynamic(this, &UNetworkModeSelectWidget::HandleServerClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &UNetworkModeSelectWidget::HandleBackClicked);
	}
}

void UNetworkModeSelectWidget::HandleLANClicked()
{
	ULANMenuWidget* LANMenu = CreateWidget<ULANMenuWidget>(GetOwningPlayer(), ULANMenuWidget::StaticClass());
	OpenChildMenu(LANMenu);
}

void UNetworkModeSelectWidget::HandleServerClicked()
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("Server Game is not implemented yet.")));
	}
}
