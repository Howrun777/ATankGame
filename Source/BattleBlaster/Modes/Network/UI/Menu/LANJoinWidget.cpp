#include "Modes/Network/UI/Menu/LANJoinWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/Networking/BattleBlasterSessionSubsystem.h"

TSharedRef<SWidget> ULANJoinWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("Join LAN Game"), TEXT("Enter host address and port."), ContentBox);

	IPTextBox = AddEditableTextBox(ContentBox, TEXT("IP"), TEXT("127.0.0.1"));
	PortTextBox = AddEditableTextBox(ContentBox, TEXT("Port"), TEXT("7777"));
	StatusText = AddMenuText(ContentBox, TEXT(""), 17, FLinearColor(0.9f, 0.72f, 0.35f, 1.0f), 14.0f);
	JoinButton = AddMenuButton(ContentBox, TEXT("Join"));
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
