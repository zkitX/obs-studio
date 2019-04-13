#include <QToolTip>
#include "media-controls.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

MediaSlider::MediaSlider(QWidget *parent)
{
	setParent(parent);
}

MediaSlider::~MediaSlider()
{
	deleteLater();
}

void MediaSlider::mouseReleaseEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls*>(parent());

	int val = minimum() + ((maximum() - minimum()) * event->x()) / width();

	mouseDown = false;
	media->MediaPlayPause(false);
	media->MediaSeekTo(val);

	event->accept();
}

void MediaSlider::mousePressEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls*>(parent());
 
	media->MediaPlayPause(true);
	mouseDown = true;
	event->accept();
}

void MediaSlider::mouseMoveEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls*>(parent());

	int val = minimum() + ((maximum() - minimum()) * event->x()) / width();
	float seconds = (((float)val / 100.0f) * media->GetMediaDuration()) /
			1000.0f;

	QToolTip::showText(QCursor::pos(), media->FormatSeconds((int)seconds),
			this);

	if (mouseDown) {
		setValue(val);
	}

	event->accept();
}

MediaControls::MediaControls(QWidget *parent, OBSSource source_)
	: QWidget (parent),
	  source  (source_)
{
	main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());

	QVBoxLayout *vLayout = new QVBoxLayout();
	QHBoxLayout *layout = new QHBoxLayout();
	QHBoxLayout *topLayout = new QHBoxLayout();

	nameLabel = new QLabel();
	timerLabel = new QLabel();
	durationLabel = new QLabel();
	nameLabel->setProperty("themeID", "mediaControlWidgets");
	timerLabel->setProperty("themeID", "mediaControlWidgets");
	durationLabel->setProperty("themeID", "mediaControlWidgets");

	SetName(QT_UTF8(obs_source_get_name(source)));

	playPauseButton = new QPushButton();
	stopButton = new QPushButton();
	stopButton->setProperty("themeID", "stopIcon");
	nextButton = new QPushButton();
	nextButton->setProperty("themeID", "nextIcon");
	previousButton = new QPushButton();
	previousButton->setProperty("themeID", "previousIcon");
	configButton = new QPushButton();
	configButton->setProperty("themeID", "configIconSmall");
	configButton->setStyleSheet("background: transparent; border:none;");

	playPauseButton->setFixedSize(32, 32);
	stopButton->setFixedSize(32, 32);
	nextButton->setFixedSize(32, 32);
	previousButton->setFixedSize(32, 32);
	configButton->setFixedSize(32, 32);

	slider = new MediaSlider();
	slider->setProperty("themeID", "mediaControlWidgets");
	slider->setOrientation(Qt::Horizontal);
	slider->setMouseTracking(true);
	slider->setMinimum(0);
	slider->setMaximum(100);

	timer = new QTimer();

	topLayout->addWidget(timerLabel);
	topLayout->addWidget(slider);
	topLayout->addWidget(durationLabel);
	topLayout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(playPauseButton);
	layout->addWidget(previousButton);
	layout->addWidget(stopButton);
	layout->addWidget(nextButton);
	layout->addStretch();
	layout->addWidget(configButton);
	layout->setContentsMargins(0, 0, 0, 0);

	vLayout->addWidget(nameLabel);
	vLayout->addLayout(topLayout);
	vLayout->addLayout(layout);
	vLayout->setContentsMargins(4, 4, 4, 4);

	setLayout(vLayout);

	connect(playPauseButton, SIGNAL(clicked()), this,
			SLOT(MediaPlayPause()));
	connect(stopButton, SIGNAL(clicked()), this,
			SLOT(MediaStop()));
	connect(nextButton, SIGNAL(clicked()), this,
			SLOT(MediaNext()));
	connect(previousButton, SIGNAL(clicked()), this,
			SLOT(MediaPrevious()));
	connect(configButton, SIGNAL(clicked()), this,
			SLOT(ShowConfigMenu()));
	connect(timer, SIGNAL(timeout()), this,
			SLOT(Update()));

	StartTimer();
}

MediaControls::~MediaControls()
{
	deleteLater();
}

OBSSource MediaControls::GetSource()
{
	return source;
}

QString MediaControls::GetName()
{
	return nameLabel->text();
}

void MediaControls::SetName(const QString &newName)
{
	nameLabel->setText(newName);
}

void MediaControls::StartTimer()
{
	if (!timer->isActive())
		timer->start(250);
}

void MediaControls::StopTimer()
{
	if (timer->isActive()) {
		timer->stop();
		Update();
	}
}

int64_t MediaControls::GetMediaTime()
{
	return obs_source_media_get_time(source);
}

int64_t MediaControls::GetMediaDuration()
{
	return obs_source_media_get_duration(source);
}

void MediaControls::SetSliderPosition()
{
	float sliderPosition = ((float)GetMediaTime() /
			(float)GetMediaDuration()) * 100.0f;

	slider->setValue((int)sliderPosition);
	SetTimerLabel();
}

void MediaControls::SetTimerLabel()
{
	timerLabel->setText(FormatSeconds(
			(int)((float)GetMediaTime() / 1000.0f)));
}

void MediaControls::SetDurationLabel()
{
	durationLabel->setText(FormatSeconds(
			(int)((float)GetMediaDuration() / 1000.0f)));
}

void MediaControls::MediaSeekTo(int value)
{
	float ms = ((float)value / 100.0f) *
			obs_source_media_get_duration(source);

	obs_source_media_set_time(source, (int64_t)ms);
	SetTimerLabel();
}

void MediaControls::Update()
{
	SetDurationLabel();
	SetSliderPosition();

	obs_media_state state = obs_source_media_get_state(source);

	if (state == prevState)
		return;

	switch (state) {
	case OBS_MEDIA_STATE_PLAYING:
		playPauseButton->setProperty("themeID", "pauseIcon");
		slider->setEnabled(true);
		setEnabled(true);
		break;
	case OBS_MEDIA_STATE_PAUSED:
		playPauseButton->setProperty("themeID", "playIcon");
		setEnabled(true);
		slider->setEnabled(true);
		break;
	case OBS_MEDIA_STATE_STOPPED:
		ResetControls();
		setEnabled(true);
		break;
	case OBS_MEDIA_STATE_ENDED:
		playPauseButton->setProperty("themeID", "restartIcon");
		setEnabled(true);
		break;
	case OBS_MEDIA_STATE_NONE:
		playPauseButton->setProperty("themeID", "playIcon");
		setEnabled(false);
		break;
	default:
		break;
	}

	playPauseButton->style()->unpolish(playPauseButton);
	playPauseButton->style()->polish(playPauseButton);

	prevState = state;
}

QString MediaControls::FormatSeconds(int totalSeconds)
{
	int seconds      = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes      = totalMinutes % 60;
	int hours        = totalMinutes / 60;

	QString text;
	text.sprintf("%02d:%02d:%02d", hours, minutes, seconds);

	return text;
}

void MediaControls::MediaPlayPause()
{
	playPauseButton->blockSignals(true);

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_STOPPED || state == OBS_MEDIA_STATE_ENDED)
		obs_source_media_restart(source);
	else if (state == OBS_MEDIA_STATE_PLAYING)
		obs_source_media_play_pause(source, true);
	else if (state == OBS_MEDIA_STATE_PAUSED)
		obs_source_media_play_pause(source, false);

	playPauseButton->blockSignals(false);
}

void MediaControls::MediaPlayPause(bool pause)
{
	obs_source_media_play_pause(source, pause);

	if (pause)
		StopTimer();
	else
		StartTimer();
}

void MediaControls::MediaStop()
{
	stopButton->blockSignals(true);
	obs_source_media_stop(source);
	stopButton->blockSignals(false);
}

void MediaControls::MediaNext()
{
	nextButton->blockSignals(true);
	obs_source_media_next(source);
	nextButton->blockSignals(false);
}

void MediaControls::MediaPrevious()
{
	previousButton->blockSignals(true);
	obs_source_media_previous(source);
	previousButton->blockSignals(false);
}

void MediaControls::ResetControls()
{
	playPauseButton->setProperty("themeID", "restartIcon");;
	slider->setValue(0);
	slider->setEnabled(false);
	timerLabel->setText(FormatSeconds(0));
	durationLabel->setText(FormatSeconds(0));
}

void MediaControls::ShowConfigMenu()
{
	QAction *unhideAllAction = new QAction(QTStr("UnhideAll"), this);
	QAction *hideAction = new QAction(QTStr("Hide"), this);

	QAction *filtersAction = new QAction(QTStr("Filters"), this);
	QAction *propertiesAction = new QAction(QTStr("Properties"), this);

	connect(hideAction, SIGNAL(triggered()),
			this, SLOT(HideMediaControls()));
	connect(unhideAllAction, SIGNAL(triggered()),
			this, SLOT(UnhideAllMediaControls()));

	connect(filtersAction, SIGNAL(triggered()),
			this, SLOT(MediaFilters()));
	connect(propertiesAction, SIGNAL(triggered()),
			this, SLOT(MediaProperties()));

	QMenu popup;
	popup.addAction(unhideAllAction);
	popup.addAction(hideAction);
	popup.addSeparator();
	popup.addAction(filtersAction);
	popup.addAction(propertiesAction);
	popup.exec(QCursor::pos());
}

void MediaControls::UnhideAllMediaControls()
{
	main->UnhideMediaControls();
}

void MediaControls::HideMediaControls()
{
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "hideMediaControls", true);
	obs_data_release(settings);
 
	main->HideMediaControls(source);
}

void MediaControls::MediaFilters()
{
	main->CreateFiltersWindow(source);
}

void MediaControls::MediaProperties()
{
	main->CreatePropertiesWindow(source);
}
