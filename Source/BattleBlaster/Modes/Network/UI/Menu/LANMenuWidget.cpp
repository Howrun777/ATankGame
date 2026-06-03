#include "Modes/Network/UI/Menu/LANMenuWidget.h"

#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Modes/Network/UI/Menu/LANHostSettingsWidget.h"
#include "Modes/Network/UI/Menu/LANJoinWidget.h"

TSharedRef<SWidget> ULANMenuWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("LAN Game"), TEXT("Create a room on this machine or connect directly to another host."), ContentBox);

	HostButton = AddMenuButton(ContentBox, TEXT("Host Game"), 14.0f);
	JoinButton = AddMenuButton(ContentBox, TEXT("Join by IP"));
	BackButton = AddMenuButton(ContentBox, TEXT("Back"));

	return Super::RebuildWidget();
}

void ULANMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &ULANMenuWidget::HandleHostClicked);
	}
	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &ULANMenuWidget::HandleJoinClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULANMenuWidget::HandleBackClicked);
	}
}

void ULANMenuWidget::HandleHostClicked()
{
	ULANHostSettingsWidget* HostSettings = CreateWidget<ULANHostSettingsWidget>(GetOwningPlayer(), ULANHostSettingsWidget::StaticClass());
	OpenChildMenu(HostSettings);
}

void ULANMenuWidget::HandleJoinClicked()
{
	ULANJoinWidget* JoinWidget = CreateWidget<ULANJoinWidget>(GetOwningPlayer(), ULANJoinWidget::StaticClass());
	OpenChildMenu(JoinWidget);
}
