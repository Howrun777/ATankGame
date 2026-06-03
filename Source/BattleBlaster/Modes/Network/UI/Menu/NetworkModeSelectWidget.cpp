#include "Modes/Network/UI/Menu/NetworkModeSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Modes/Network/UI/Menu/LANMenuWidget.h"

TSharedRef<SWidget> UNetworkModeSelectWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Network Game"), TEXT("Choose how this machine should connect to a multiplayer session."), ContentBox);

	LANButton = AddMenuButton(ContentBox, TEXT("LAN Game"), 14.0f);
	ServerButton = AddMenuButton(ContentBox, TEXT("Server Game (Not Available)"));
	StatusText = AddMenuText(ContentBox, TEXT("LAN mode uses local IP hosting and direct IP joining."), 17, FLinearColor(0.78f, 0.82f, 0.84f, 1.0f), 22.0f);
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
