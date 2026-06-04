#include "Modes/Network/UI/Menu/NetworkModeSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Modes/Network/UI/Menu/LANMenuWidget.h"

TSharedRef<SWidget> UNetworkModeSelectWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Network Game"), TEXT("Choose a multiplayer connection path."), ContentBox);

	AddSectionHeader(ContentBox, TEXT("CONNECTION TYPE"));
	LANButton = AddMenuCardButton(ContentBox, TEXT("LAN Game"), TEXT("Host locally or join by direct IP."), TEXT("READY"), true);
	ServerButton = AddMenuCardButton(ContentBox, TEXT("Server Game"), TEXT("Dedicated server flow, planned for later."), TEXT("PLANNED"), false);
	StatusText = AddNoticeText(ContentBox, TEXT("LAN mode is available now."), FLinearColor(0.08f, 0.74f, 0.64f, 1.0f), 18.0f);
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
