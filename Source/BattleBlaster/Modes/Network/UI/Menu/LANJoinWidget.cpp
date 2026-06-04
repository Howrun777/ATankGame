#include "Modes/Network/UI/Menu/LANJoinWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/Networking/BattleBlasterSessionSubsystem.h"

TSharedRef<SWidget> ULANJoinWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Join LAN Game"), TEXT("Enter the host address and port."), ContentBox);

	AddSectionHeader(ContentBox, TEXT("DIRECT CONNECT"));
	IPTextBox = AddEditableTextBox(ContentBox, TEXT("IP Address"), TEXT("127.0.0.1"));
	PortTextBox = AddEditableTextBox(ContentBox, TEXT("Port"), TEXT("7777"));
	StatusText = AddNoticeText(ContentBox, TEXT("Same machine: 127.0.0.1. LAN: use the host IP."), FLinearColor(0.08f, 0.74f, 0.64f, 1.0f), 18.0f);
	JoinButton = AddMenuButton(ContentBox, TEXT("Join Game"), 14.0f);
	BackButton = AddMenuButton(ContentBox, TEXT("Back"));

	return Super::RebuildWidget();
}

void ULANJoinWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ULANJoinWidget::HandleJoinClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULANJoinWidget::HandleBackClicked);
	}
}

void ULANJoinWidget::HandleJoinClicked()
{
	const FString IP = IPTextBox ? IPTextBox->GetText().ToString() : TEXT("");
	const FString Port = PortTextBox ? PortTextBox->GetText().ToString() : TEXT("7777");

	if (IP.IsEmpty())
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("IP is empty.")));
		}
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBattleBlasterSessionSubsystem* SessionSubsystem = GI->GetSubsystem<UBattleBlasterSessionSubsystem>())
		{
			SessionSubsystem->JoinByIpAndPort(IP, Port);
		}
	}
}
