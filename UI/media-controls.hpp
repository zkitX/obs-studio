#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QSlider>
#include <QMouseEvent>
#include <QLabel>
#include <QTimer>
#include <obs.hpp>

class OBSBasic;

class MediaSlider : public QSlider {
	Q_OBJECT

public:
	explicit MediaSlider(QWidget *parent = 0);
	virtual ~MediaSlider();

private:
	bool mouseDown = false;

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
};

class MediaControls : public QWidget {
	Q_OBJECT

	friend MediaSlider;

private:
	OBSBasic      *main;
	MediaSlider   *slider;
	QLabel        *nameLabel;
	QLabel        *timerLabel;
	QLabel        *durationLabel;
	QTimer        *timer;
	QPushButton   *playPauseButton;
	QPushButton   *stopButton;
	QPushButton   *nextButton;
	QPushButton   *previousButton;
	QPushButton   *configButton;
	OBSSignal     selectSignal;
	OBSSignal     removeSignal;
	OBSSignal     channelChangedSignal;
	enum obs_media_state prevState;

	QString FormatSeconds(int totalSeconds);
	void SetPlayPauseIcon();
	int64_t GetMediaTime();
	int64_t GetMediaDuration();
	void SetSliderPosition();
	void SetTimerLabel();
	void SetDurationLabel();
	void MediaSeekTo(int value);
	void ResetControls();
	void MediaPlayPause(bool pause);
	void StartTimer();
	void StopTimer();

private slots:
	void MediaPlayPause();
	void MediaStop();
	void MediaNext();
	void MediaPrevious();
	void Update();

	void ShowConfigMenu();
	void UnhideAllMediaControls();
	void HideMediaControls();
	void MediaFilters();
	void MediaProperties();

public:
	OBSSource     source;

	MediaControls(QWidget *parent, OBSSource source_);
	~MediaControls();

	void SetScene(OBSScene scene);
	void SetName(const QString &newName);
	OBSSource GetSource();
	QString GetName();
};
