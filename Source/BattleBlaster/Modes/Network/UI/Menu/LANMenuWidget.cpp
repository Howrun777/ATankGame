#include "Modes/Network/UI/Menu/LANMenuWidget.h"

#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Modes/Network/UI/Menu/LANHostSettingsWidget.h"
#include "Modes/Network/UI/Menu/LANJoinWidget.h"

TSharedRef<SWidget> ULANMenuWidget::RebuildWidget()
{
	UVerticalBox* ContentBox = nullptr;
	BuildMenuRoot(TEXT("LAN Game"), TEXT("Create or join a local network match."), ContentBox);

	AddSectionHeader(ContentBox, TEXT("LAN SESSION"));
	HostButton = AddMenuCardButton(ContentBox, TEXT("Host Game"), TEXT("Create a room and choose match rules."), TEXT("CREATE"), true);
	JoinButton = AddMenuCardButton(ContentBox, TEXT("Join by IP"), TEXT("Connect to an existing host address."), TEXT("CONNECT"), false);
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
	TSubclassOf<ULANHostSettingsWidget> ClassToSpawn = HostSettingsWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ULANHostSettingsWidget::StaticClass();
	}

	ULANHostSettingsWidget* HostSettings = CreateWidget<ULANHostSettingsWidget>(GetOwningPlayer(), ClassToSpawn);
	OpenChildMenu(HostSettings);
}

void ULANMenuWidget::HandleJoinClicked()
{
	TSubclassOf<ULANJoinWidget> ClassToSpawn = JoinWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = ULANJoinWidget::StaticClass();
	}

	ULANJoinWidget* JoinWidget = CreateWidget<ULANJoinWidget>(GetOwningPlayer(), ClassToSpawn);
	OpenChildMenu(JoinWidget);
}
