#include "notifications-widget.hpp"
#include <QDesktopServices>

OBSNotifications::OBSNotifications(NotifyType type,
				   QString text,
				   QString buttonText,
				   QString url,
				   QFrame *parent)
	: QFrame (parent),
	  buttonURL (url)
{
	layout = new QHBoxLayout();
	layout->setContentsMargins(4, 4 ,4, 4);
	notificationText = new QLabel(text);
	notificationText->setProperty("themeID", "notifyText");

	closeButton = new ClickableLabel();
	closeButton->setFixedSize(16, 16);
	closeButton->setProperty("themeID", "notifyCloseButton");

	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

	if (!buttonText.isNull() && !url.isNull()) {
		button = new QPushButton(buttonText);
		button->setMaximumWidth(notificationText->width());
		button->setProperty("themeID", "notifyButton");
		connect(button, SIGNAL(clicked()), this, SLOT(ButtonClicked()));
	}

	icon = new QLabel();

	if (type == NotifyType::Info) {
		icon->setProperty("themeID", "notifyIconInfo");
		setProperty("themeID", "notifyInfo");
	} else if (type == NotifyType::Warning) {
		icon->setProperty("themeID", "notifyIconWarning");
		setProperty("themeID", "notifyWarning");
	} else if (type == NotifyType::Error) {
		icon->setProperty("themeID", "notifyIconError");
		setProperty("themeID", "notifyError");
	}

	icon->setFixedSize(32, 32);

	layout->addStretch();
	layout->addWidget(icon);
	layout->addWidget(notificationText);

	if (!buttonText.isNull() && !url.isNull())
		layout->addWidget(button);

	layout->addStretch();
	layout->addWidget(closeButton);

	setLayout(layout);
}

OBSNotifications::~OBSNotifications()
{
	deleteLater();
}

void OBSNotifications::ButtonClicked()
{
	QDesktopServices::openUrl(QUrl(buttonURL));
}
