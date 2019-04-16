#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include "clickable-label.hpp"

enum class NotifyType {
	Info,
	Warning,
	Error
};

class OBSNotifications : public QFrame {
	Q_OBJECT

private:
	QHBoxLayout    *layout;
	QPushButton    *button;
	QLabel         *notificationText;
	QLabel         *icon;
	ClickableLabel *closeButton;

	QString buttonURL;

	NotifyType type;

private slots:
	void ButtonClicked();

public:
	OBSNotifications(NotifyType type,
			QString text,
			QString buttonText = nullptr,
			QString url = nullptr,
			QFrame *parent = 0);

	~OBSNotifications();
};
