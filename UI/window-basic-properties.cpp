/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "obs-app.hpp"
#include "window-basic-properties.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"
#include "properties-view.hpp"
#include "vertical-scroll-area.hpp"

#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>

using namespace std;

OBSBasicProperties::OBSBasicProperties(QWidget *parent, OBSSource source_,
			PropertiesType type)
	: QDialog                (parent),
	  preview                (new OBSQTDisplay(this)),
	  main                   (qobject_cast<OBSBasic*>(parent)),
	  acceptClicked          (false),
	  source                 (source_),
	  removedSignal          (obs_source_get_signal_handler(source),
	                          "remove", OBSBasicProperties::SourceRemoved,
	                          this),
	  renamedSignal          (obs_source_get_signal_handler(source),
	                          "rename", OBSBasicProperties::SourceRenamed,
	                          this),
	  oldSettings            (obs_data_create()),
	  buttonBox              (new QDialogButtonBox(this))
{
	item = main->GetCurrentSceneItem();

	int cx = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
			"cx");
	int cy = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
			"cy");

	buttonBox->setObjectName(QStringLiteral("buttonBox"));
	buttonBox->setStandardButtons(QDialogButtonBox::Ok |
	                              QDialogButtonBox::Cancel |
	                              QDialogButtonBox::RestoreDefaults);

	buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("OK"));
	buttonBox->button(QDialogButtonBox::Cancel)->setText(QTStr("Cancel"));
	buttonBox->button(QDialogButtonBox::RestoreDefaults)->
		setText(QTStr("Defaults"));

	if (cx > 400 && cy > 400)
		resize(cx, cy);
	else
		resize(720, 580);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QMetaObject::connectSlotsByName(this);

	/* The OBSData constructor increments the reference once */
	obs_data_release(oldSettings);

	OBSData settings = obs_source_get_settings(source);
	obs_data_apply(oldSettings, settings);
	obs_data_release(settings);

	setStyleSheet("QTabBar::tab:first {background-color: none; \
					   font-weight: bold; \
					   border: none; \
					   padding: 5px; \
					   min-width: 50px; \
					   margin: 1px;}");

	tabsLeft = new QTabBar(this);
	QWidget *bottom = new QWidget(this);
	QHBoxLayout *tabsLayout = new QHBoxLayout();
	QScrollArea *scroll = new QScrollArea();
	QWidget *scrollContents = new QWidget();
	QVBoxLayout *vLayout = new QVBoxLayout(scrollContents);
	QVBoxLayout *vLayout2 = new QVBoxLayout();

	tabsLeft->setDrawBase(false);
	tabsLeft->setTabEnabled(0, false);

	if (type == PropertiesType::Source) {
		tabsLeft->setProperty("themeID", "propertyTabs");
		tabsLeft->addTab((QTStr("Basic.PropertiesWindow.Global")));
	}

	scroll->setWidgetResizable(true);

	filters = new OBSBasicFilters(this, source);
	filters->Init();

	if (type != PropertiesType::Scene) {
		tabsLeft->addTab(QTStr("Basic.Settings.General"));
	}

	tabsLeft->addTab(QTStr("Filters"));

	if (type != PropertiesType::Scene) {
		view = new OBSPropertiesView(settings, source,
				(PropertiesReloadCallback)obs_source_properties,
				(PropertiesUpdateCallback)obs_source_update);
		view->setMinimumHeight(150);
	}

	if (type == PropertiesType::Source) {
		transformWindow = new OBSBasicTransform(this,
				obs_sceneitem_get_scene(item));

		tabsRight = new QTabBar(this);

		tabsRight->setDrawBase(false);
		tabsRight->setTabEnabled(0, false);

		itemAdvanced = AdvancedItemTab(this);
		globalAdvanced = AdvancedGlobalTab(this);

		tabsRight->setProperty("themeID", "propertyTabs");
		tabsRight->addTab(QTStr("Basic.PropertiesWindow.ThisScene"));

		tabsRight->addTab(QTStr("Basic.MainMenu.Edit.Transform"));
		tabsRight->addTab(QTStr("Basic.Settings.Advanced"));
		connect(tabsRight, SIGNAL(tabBarClicked(int)), this,
			SLOT(TabsRightClicked(int)));
		tabsRight->show();
	}

	if (type != PropertiesType::Transition) {
		hotkeysTab = HotkeysTab(this);
		tabsLeft->addTab(QTStr("Basic.Settings.Hotkeys"));
	}

	if (type == PropertiesType::Source)
		tabsLeft->addTab(QTStr("Basic.Settings.Advanced"));

	if (type == PropertiesType::Scene) {
		trOverride = PerSceneTransitionWidget(this);
		tabsLeft->addTab(QTStr("TransitionOverride"));
	}

	preview->setMinimumSize(20, 150);
	preview->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
				QSizePolicy::Expanding));

	tabsLayout->addWidget(tabsLeft);
	tabsLayout->addStretch();

	if (type == PropertiesType::Source)
		tabsLayout->addWidget(tabsRight);

	vLayout2->addLayout(tabsLayout);
	vLayout->addWidget(filters);

	if (type != PropertiesType::Transition)
		vLayout->addWidget(hotkeysTab);

	if (type != PropertiesType::Scene)
		vLayout->addWidget(view);
	else
		vLayout->addWidget(trOverride);

	if (type == PropertiesType::Source) {
		vLayout->addWidget(globalAdvanced);
		vLayout->addWidget(itemAdvanced);
		vLayout->addWidget(transformWindow);
	}

	vLayout->addStretch();
	scroll->setWidget(scrollContents);
	vLayout2->addWidget(scroll);
	vLayout2->addStretch();
	bottom->setLayout(vLayout2);

	// Create a QSplitter to keep a unified workflow here.
	windowSplitter = new QSplitter(Qt::Orientation::Vertical, this);
	windowSplitter->addWidget(preview);
	windowSplitter->addWidget(bottom);
	windowSplitter->setChildrenCollapsible(false);
	//windowSplitter->setSizes(QList<int>({ 16777216, 150 }));
	windowSplitter->setStretchFactor(0, 3);
	windowSplitter->setStretchFactor(1, 1);

	setLayout(new QVBoxLayout(this));
	layout()->addWidget(windowSplitter);
	layout()->addWidget(buttonBox);
	layout()->setAlignment(buttonBox, Qt::AlignBottom);

	tabsLeft->show();
	installEventFilter(CreateShortcutFilter());

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));

	obs_source_inc_showing(source);

	updatePropertiesSignal.Connect(obs_source_get_signal_handler(source),
			"update_properties",
			OBSBasicProperties::UpdateProperties,
			this);

	auto addDrawCallback = [this] ()
	{
		obs_display_add_draw_callback(preview->GetDisplay(),
				OBSBasicProperties::DrawPreview, this);
	};

	enum obs_source_type sourceType = obs_source_get_type(source);
	uint32_t caps = obs_source_get_output_flags(source);
	bool drawable_type = sourceType == OBS_SOURCE_TYPE_INPUT ||
		sourceType == OBS_SOURCE_TYPE_SCENE;
	bool drawable_preview = (caps & OBS_SOURCE_VIDEO) != 0;

	if (drawable_preview && drawable_type) {
		preview->show();
		connect(preview.data(), &OBSQTDisplay::DisplayCreated,
				addDrawCallback);
	} else {
		preview->hide();
	}

	if ((caps & OBS_SOURCE_VIDEO) == 0) {
		tabsLeft->setTabEnabled(4, false);
		tabsRight->setTabEnabled(2, false);
		tabsRight->setTabEnabled(1, false);
	}

	auto ReloadHotkeys = [](void *data, calldata_t*)
	{
		auto settings = static_cast<OBSBasicProperties*>(data);
		QMetaObject::invokeMethod(settings, "ReloadHotkeys");
	};
	hotkeyRegistered.Connect(obs_get_signal_handler(), "hotkey_register",
			ReloadHotkeys, this);

	auto ReloadHotkeysIgnore = [](void *data, calldata_t *param)
	{
		auto settings = static_cast<OBSBasicProperties*>(data);
		auto key      = static_cast<obs_hotkey_t*>(
					calldata_ptr(param,"key"));
		QMetaObject::invokeMethod(settings, "ReloadHotkeys",
				Q_ARG(obs_hotkey_id, obs_hotkey_get_id(key)));
	};
	hotkeyUnregistered.Connect(obs_get_signal_handler(),
			"hotkey_unregister", ReloadHotkeysIgnore, this);

	connect(tabsLeft, SIGNAL(tabBarClicked(int)), this,
			SLOT(TabsLeftClicked(int)));

	propType = type;

	if (type != PropertiesType::Scene) {
		ShowGeneral();
	}else {
		ShowFilters();
	}

	if (type == PropertiesType::Source)
		tabsLeft->setCurrentIndex(1);
	else
		tabsLeft->setCurrentIndex(0);

	LoadOriginalSettings();
}

OBSBasicProperties::~OBSBasicProperties()
{
	obs_source_dec_showing(source);
	main->SaveProject();
}

void OBSBasicProperties::LoadOriginalSettings()
{
	obs_sceneitem_get_info(item, &originalTransform);
	obs_sceneitem_get_crop(item, &originalCrop);

	originalDeinterlaceMode = obs_source_get_deinterlace_mode(source);
	originalDeinterlaceOrder =
			obs_source_get_deinterlace_field_order(source);
	originalScaleFilter = obs_sceneitem_get_scale_filter(item);

	OBSData data = obs_source_get_private_settings(source);
	obs_data_release(data);

	originalTransition = obs_data_get_string(data, "transition");
	originalDuration = (int)obs_data_get_int(data, "transition_duration");
}

void OBSBasicProperties::SaveOriginalSettings()
{
	obs_sceneitem_set_info(item, &originalTransform);
	obs_sceneitem_set_crop(item, &originalCrop);

	obs_sceneitem_set_scale_filter(item, originalScaleFilter);
	obs_source_set_deinterlace_mode(source, originalDeinterlaceMode);
	obs_source_set_deinterlace_field_order(source,
			originalDeinterlaceOrder);

	OBSData data = obs_source_get_private_settings(source);
	obs_data_release(data);

	obs_data_set_string(data, "transition", originalTransition);
	obs_data_set_int(data, "transition_duration", originalDuration);
}

void OBSBasicProperties::ShowGeneral()
{
	view->show();
	filters->hide();

	if (propType != PropertiesType::Transition) {
		transformWindow->hide();
		hotkeysTab->hide();
	}

	if (propType == PropertiesType::Source) {
		itemAdvanced->hide();
		globalAdvanced->hide();
	}
}

void OBSBasicProperties::ShowFilters()
{
	filters->show();

	if (propType != PropertiesType::Scene) {
		view->hide();

		if (propType != PropertiesType::Transition)
			transformWindow->hide();
	} else {
		trOverride->hide();
	}

	if (propType != PropertiesType::Transition)
		hotkeysTab->hide();

	if (propType == PropertiesType::Source) {
		itemAdvanced->hide();
		globalAdvanced->hide();
	}
}

void OBSBasicProperties::ShowTransform()
{
	if (propType != PropertiesType::Scene) {
		transformWindow->show();
		view->hide();
	}

	filters->hide();
	hotkeysTab->hide();

	if (propType == PropertiesType::Source) {
		itemAdvanced->hide();
		globalAdvanced->hide();
	}
}

void OBSBasicProperties::ShowHotkeys()
{
	hotkeysTab->show();

	if (propType != PropertiesType::Scene)
		view->hide();
	else
		trOverride->hide();

	filters->hide();

	if (propType == PropertiesType::Source) {
		itemAdvanced->hide();
		globalAdvanced->hide();
		transformWindow->hide();
	}
}

void OBSBasicProperties::ShowGlobalAdvanced()
{
	globalAdvanced->show();
	view->hide();
	filters->hide();
	hotkeysTab->hide();
	transformWindow->hide();
	itemAdvanced->hide();
}

void OBSBasicProperties::ShowItemAdvanced()
{
	itemAdvanced->show();
	globalAdvanced->hide();
	view->hide();
	filters->hide();
	hotkeysTab->hide();
	transformWindow->hide();
}

void OBSBasicProperties::ShowPerSceneTransition()
{
	trOverride->show();
	filters->hide();
	hotkeysTab->hide();
}

void OBSBasicProperties::HideLeft()
{
	tabsLeft->setCurrentIndex(0);
}

void OBSBasicProperties::HideRight()
{
	if (tabsRight)
		tabsRight->setCurrentIndex(0);
}

void OBSBasicProperties::TabsLeftClicked(int index)
{
	if (propType == PropertiesType::Source) {
		if (index == 1)
			ShowGeneral();
		else if (index == 2)
			ShowFilters();
		else if (index == 3)
			ShowHotkeys();
		else if (index == 4)
			ShowGlobalAdvanced();

		HideRight();
	} else if (propType == PropertiesType::Scene) {
		if (index == 0)
			ShowFilters();
		else if (index == 1)
			ShowHotkeys();
		else if (index == 2)
			ShowPerSceneTransition();
	} else if (propType == PropertiesType::Transition) {
		if (index == 0)
			ShowGeneral();
		else if (index == 1)
			ShowFilters();
	}
}

void OBSBasicProperties::TabsRightClicked(int index)
{
	if (index == 0)
		HideLeft();
	else if (index == 1)
		ShowTransform();
	else if (index == 2)
		ShowItemAdvanced();

	HideLeft();
}

void OBSBasicProperties::SourceRemoved(void *data, calldata_t *params)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data),
			"close");

	UNUSED_PARAMETER(params);
}

void OBSBasicProperties::SourceRenamed(void *data, calldata_t *params)
{
	const char *name = calldata_string(params, "new_name");
	QString title = QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data),
	                "setWindowTitle", Q_ARG(QString, title));
}

void OBSBasicProperties::UpdateProperties(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data)->view,
			"ReloadProperties");
}

void OBSBasicProperties::ResetSourcesDialog()
{
	if (view->isVisible()) {
		obs_data_t *settings = obs_source_get_settings(source);
		obs_data_clear(settings);
		obs_data_release(settings);

		if (!view->DeferUpdate()) {
			obs_source_update(source, nullptr);

			view->RefreshProperties();
		}
	}

	if (filters->isVisible()) {
		filters->ResetFilters();
	} else if (transformWindow->isVisible()) {
		transformWindow->ResetTransform();
	} else if (globalAdvanced->isVisible()) {
		if (deinterlace)
			deinterlace->setCurrentIndex(0);
		if (order)
			order->setCurrentIndex(0);
	} else if (itemAdvanced->isVisible()) {
		if (sf)
			sf->setCurrentIndex(0);
	} else if (hotkeysTab->isVisible()) {
		SetHotkeyDefaults();
	}
}

void OBSBasicProperties::ResetScenesDialog()
{
	if (filters->isVisible()) {
		filters->ResetFilters();
	} else if (trOverride->isVisible()) {
		combo->setCurrentIndex(0);
		duration->setValue(300);
	} else if (hotkeysTab->isVisible()) {
		SetHotkeyDefaults();
	}
}

void OBSBasicProperties::ResetTransitionsDialog()
{
	if (view->isVisible()) {
		obs_data_t *settings = obs_source_get_settings(source);
		obs_data_clear(settings);
		obs_data_release(settings);

		if (!view->DeferUpdate()) {
			obs_source_update(source, nullptr);

			view->RefreshProperties();
		} else if (filters->isVisible()) {
			filters->ResetFilters();
		}
	}
}

void OBSBasicProperties::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::AcceptRole) {
		acceptClicked = true;

		if (view && propType != PropertiesType::Scene) {
			if (view->DeferUpdate())
				view->UpdateSettings();
			if (propType == PropertiesType::Source)
				SaveHotkeySettings();
		} else {
			SaveHotkeySettings();
		}

		close();

	} else if (val == QDialogButtonBox::RejectRole) {
		if (view && propType != PropertiesType::Scene) {
			obs_data_t *settings = obs_source_get_settings(source);
			obs_data_clear(settings);
			obs_data_release(settings);

			if (view->DeferUpdate())
				obs_data_apply(settings, oldSettings);
			else
				obs_source_update(source, oldSettings);
		}

		SaveOriginalSettings();

		close();

	} else if (val == QDialogButtonBox::ResetRole) {
		if (propType == PropertiesType::Source) {
			ResetSourcesDialog();
		} else if (propType == PropertiesType::Scene) {
			ResetScenesDialog();
		} else if (propType == PropertiesType::Transition) {
			ResetTransitionsDialog();
		}
	}
}

void OBSBasicProperties::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasicProperties *window = static_cast<OBSBasicProperties*>(data);

	if (!window->source)
		return;

	uint32_t sourceCX = max(obs_source_get_width(window->source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(window->source), 1u);

	int   x, y;
	int   newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY),
			-100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->source);

	gs_projection_pop();
	gs_viewport_pop();
}

void OBSBasicProperties::Cleanup()
{
	config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cx",
			width());
	config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cy",
			height());

	obs_display_remove_draw_callback(preview->GetDisplay(),
		OBSBasicProperties::DrawPreview, this);
}

void OBSBasicProperties::reject()
{
	if (!acceptClicked && (CheckSettings() != 0)) {
		if (!ConfirmQuit()) {
			return;
		}
	}

	Cleanup();
	done(0);
}

void OBSBasicProperties::closeEvent(QCloseEvent *event)
{
	if (!acceptClicked && (CheckSettings() != 0)) {
		if (!ConfirmQuit()) {
			event->ignore();
			return;
		}
	}

	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	Cleanup();
}

void OBSBasicProperties::Init()
{
	show();
}

int OBSBasicProperties::CheckSettings()
{
	OBSData currentSettings = obs_source_get_settings(source);
	const char *oldSettingsJson = obs_data_get_json(oldSettings);
	const char *currentSettingsJson = obs_data_get_json(currentSettings);

	int ret = strcmp(currentSettingsJson, oldSettingsJson);

	obs_data_release(currentSettings);
	return ret;
}

bool OBSBasicProperties::ConfirmQuit()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this,
			QTStr("Basic.PropertiesWindow.ConfirmTitle"),
			QTStr("Basic.PropertiesWindow.Confirm"),
			QMessageBox::Save | QMessageBox::Discard |
			QMessageBox::Cancel);

	switch (button) {
	case QMessageBox::Save:
		acceptClicked = true;
		if (view->DeferUpdate())
			view->UpdateSettings();
		// Do nothing because the settings are already updated
		break;
	case QMessageBox::Discard:
		obs_source_update(source, oldSettings);
		break;
	case QMessageBox::Cancel:
		return false;
		break;
	default:
		/* If somehow the dialog fails to show, just default to
		 * saving the settings. */
		break;
	}
	return true;
}

void OBSBasicProperties::OpenTransformTab()
{
	tabsRight->setCurrentIndex(1);
	ShowTransform();

	tabsLeft->setCurrentIndex(0);
}

QWidget *OBSBasicProperties::PerSceneTransitionWidget(QWidget *parent)
{
	OBSData data = obs_source_get_private_settings(source);
	obs_data_release(data);

	obs_data_set_default_int(data, "transition_duration", 300);
	const char *curTransition = obs_data_get_string(data, "transition");
	int curDuration = (int)obs_data_get_int(data, "transition_duration");

	QWidget *w = new QWidget(parent);
	combo = new QComboBox(w);
	duration = new QSpinBox(w);
	QLabel *trLabel = new QLabel(QTStr("Transition"));
	QLabel *durationLabel = new QLabel(QTStr("Basic.TransitionDuration"));

	duration->setMinimum(50);
	duration->setSuffix("ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(curDuration);

	QFormLayout *layout = new QFormLayout();
	layout->setLabelAlignment(Qt::AlignRight);

	layout->addRow(trLabel, combo);
	layout->addRow(durationLabel, duration);

	combo->addItem("None");

	const char *name = nullptr;

	for (int i = 0; i < main->ui->transitions->count(); i++) {
		OBSSource tr;
		tr = main->GetTransitionComboItem(main->ui->transitions, i);
		name = obs_source_get_name(tr);

		combo->addItem(name);
	}

	int index = combo->findText(curTransition);
	if (index != -1) {
		combo->setCurrentIndex(index);
	}

	auto setTransition = [this] (int idx)
	{
		OBSData data = obs_source_get_private_settings(source);
		obs_data_release(data);

		if (idx == -1) {
			obs_data_set_string(data, "transition", "");
			return;
		}

		OBSSource tr = main->GetTransitionComboItem(
				main->ui->transitions, idx - 1);
		const char *name = obs_source_get_name(tr);

		obs_data_set_string(data, "transition", name);
	};

	auto setDuration = [this] (int duration)
	{
		OBSData data = obs_source_get_private_settings(source);
		obs_data_release(data);

		obs_data_set_int(data, "transition_duration", duration);
	};

	connect(combo, (void
			(QComboBox::*)(int))&QComboBox::currentIndexChanged,
			setTransition);
	connect(duration, (void (QSpinBox::*)(int))&QSpinBox::valueChanged,
			setDuration);

	w->setLayout(layout);
	return w;
}

void OBSBasicProperties::SetDeinterlacingMode(int index)
{
	if (index == 0)
		order->setEnabled(false);
	else
		order->setEnabled(true);

	obs_deinterlace_mode mode = (obs_deinterlace_mode)(index);

	obs_source_set_deinterlace_mode(source, mode);
}

void OBSBasicProperties::SetDeinterlacingOrder(int index)
{
	obs_deinterlace_field_order deinterlaceOrder =
			(obs_deinterlace_field_order)(index);

	obs_source_set_deinterlace_field_order(source, deinterlaceOrder);
}

void OBSBasicProperties::SetScaleFilter(int index)
{
	obs_scale_type mode = (obs_scale_type)(index);

	obs_sceneitem_set_scale_filter(item, mode);
}

OBSSource OBSBasicProperties::GetSource()
{
	return source;
}

QWidget *OBSBasicProperties::AdvancedItemTab(QWidget *parent)
{
	QWidget *w = new QWidget(parent);

	sf = new QComboBox(w);

	QLabel *sfLabel = new QLabel(QTStr("ScaleFiltering"));

	QFormLayout *layout = new QFormLayout();
	layout->setLabelAlignment(Qt::AlignRight);

	obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);

#define ADD_SF_MODE(name, mode) \
	sf->addItem(QTStr("" name)); \
	sf->setProperty("mode", (int)mode);

	ADD_SF_MODE("Disable",                 OBS_SCALE_DISABLE);
	ADD_SF_MODE("ScaleFiltering.Point",    OBS_SCALE_POINT);
	ADD_SF_MODE("ScaleFiltering.Bilinear", OBS_SCALE_BILINEAR);
	ADD_SF_MODE("ScaleFiltering.Bicubic",  OBS_SCALE_BICUBIC);
	ADD_SF_MODE("ScaleFiltering.Lanczos",  OBS_SCALE_LANCZOS);
#undef ADD_SF_MODE

	sf->setCurrentIndex((int)scaleFilter);
	connect(sf, SIGNAL(currentIndexChanged(int)),
			this, SLOT(SetScaleFilter(int)));
	layout->addRow(sfLabel, sf);

	w->setLayout(layout);
	return w;
}

QWidget *OBSBasicProperties::AdvancedGlobalTab(QWidget *parent)
{
	uint32_t flags = obs_source_get_output_flags(source);
	bool isAsyncVideo = (flags & OBS_SOURCE_ASYNC_VIDEO) ==
			OBS_SOURCE_ASYNC_VIDEO;

	obs_deinterlace_mode deinterlaceMode =
		obs_source_get_deinterlace_mode(source);
	obs_deinterlace_field_order deinterlaceOrder =
		obs_source_get_deinterlace_field_order(source);

	QWidget *w = new QWidget(parent);

	deinterlace = new QComboBox(w);
	order = new QComboBox(w);

	QLabel *deinterlaceLabel = new QLabel(QTStr("Deinterlacing.Mode"));
	QLabel *orderLabel = new QLabel(QTStr("Deinterlacing.Order"));
	QLabel *noPropsLabel =
			new QLabel(QTStr(
			"Basic.PropertiesWindow.NoProperties"));

	QFormLayout *layout = new QFormLayout();
	layout->setLabelAlignment(Qt::AlignRight);

	connect(deinterlace, SIGNAL(currentIndexChanged(int)),
			this, SLOT(SetDeinterlacingMode(int)));
	connect(order, SIGNAL(currentIndexChanged(int)),
			this, SLOT(SetDeinterlacingOrder(int)));

#define ADD_MODE(name, mode) \
	deinterlace->addItem(QTStr("" name)); \
	deinterlace->setProperty("mode", (int)mode);

	ADD_MODE("Disable",
			OBS_DEINTERLACE_MODE_DISABLE);
	ADD_MODE("Deinterlacing.Discard",
			OBS_DEINTERLACE_MODE_DISCARD);
	ADD_MODE("Deinterlacing.Retro",
			OBS_DEINTERLACE_MODE_RETRO);
	ADD_MODE("Deinterlacing.Blend",
			OBS_DEINTERLACE_MODE_BLEND);
	ADD_MODE("Deinterlacing.Blend2x",
			OBS_DEINTERLACE_MODE_BLEND_2X);
	ADD_MODE("Deinterlacing.Linear",
			OBS_DEINTERLACE_MODE_LINEAR);
	ADD_MODE("Deinterlacing.Linear2x",
			OBS_DEINTERLACE_MODE_LINEAR_2X);
	ADD_MODE("Deinterlacing.Yadif",
			OBS_DEINTERLACE_MODE_YADIF);
	ADD_MODE("Deinterlacing.Yadif2x",
			OBS_DEINTERLACE_MODE_YADIF_2X);
#undef ADD_MODE

#define ADD_ORDER(name, mode) \
	order->addItem(QTStr("Deinterlacing." name)); \
	order->setProperty("order", (int)mode);

	ADD_ORDER("TopFieldFirst",
			OBS_DEINTERLACE_FIELD_ORDER_TOP);
	ADD_ORDER("BottomFieldFirst",
			OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
#undef ADD_ORDER

	layout->addRow(deinterlaceLabel, deinterlace);
	layout->addRow(orderLabel, order);
	layout->setContentsMargins(0, 0, 0, 0);

	if (!isAsyncVideo) {
		deinterlace->hide();
		order->hide();
		deinterlaceLabel->hide();
		orderLabel->hide();

		layout->addRow(noPropsLabel);
	}

	if (((int)deinterlaceMode) == 0)
		order->setEnabled(false);
	else
		order->setEnabled(true);

	deinterlace->setCurrentIndex((int)deinterlaceMode);
	order->setCurrentIndex((int)deinterlaceOrder);

	w->setLayout(layout);
	return w;
}

void OBSBasicProperties::SetHotkeyDefaults()
{
	QList<OBSHotkeyEdit*> list = hotkeysTab->findChildren<OBSHotkeyEdit*>();

	foreach(OBSHotkeyEdit *w, list) {
		w->ClearKey();
	}
}

void OBSBasicProperties::ReloadHotkeys(obs_hotkey_id ignoreKey)
{
	LoadHotkeySettings(ignoreKey);
}

void OBSBasicProperties::HotkeysChanged()
{
	using namespace std;

	hotkeysChanged = any_of(begin(hotkeys), end(hotkeys),
			[](const pair<bool, QPointer<OBSHotkeyWidget>> &hotkey)
	{
		const auto &hw = *hotkey.second;
		return hw.Changed();
	});
}

void OBSBasicProperties::SaveHotkeySettings()
{
	const auto &config = main->Config();

	using namespace std;

	std::vector<obs_key_combination> combinations;
	for (auto &hotkey : hotkeys) {
		auto &hw = *hotkey.second;
		if (!hw.Changed())
			continue;

		hw.Save(combinations);

		if (!hotkey.first)
			continue;

		obs_data_array_t *array = obs_hotkey_save(hw.id);
		obs_data_t *data = obs_data_create();
		obs_data_set_array(data, "bindings", array);
		const char *json = obs_data_get_json(data);
		config_set_string(config, "Hotkeys", hw.name.c_str(), json);
		obs_data_release(data);
		obs_data_array_release(array);
	}
}

void OBSBasicProperties::LoadHotkeySettings(obs_hotkey_id ignoreKey)
{
	UNUSED_PARAMETER(ignoreKey);

	hotkeys.clear();

	hotkeyLayout = new QFormLayout();
	hotkeyLayout->setItem(hotkeyLayout->rowCount(),
			QFormLayout::SpanningRole,
			new QSpacerItem(0, 0));

	obs_enum_hotkey_bindings([](void *data,
				size_t, obs_hotkey_binding_t *binding)
	{
		OBSBasicProperties *window =
				reinterpret_cast<OBSBasicProperties*>(data);

		window->keys[
			obs_hotkey_binding_get_hotkey_id(binding)].emplace_back(
			obs_hotkey_binding_get_key_combination(binding));

		return true;
	}, this);

	auto SetHotkeys = [] (void *data, obs_hotkey_id id, obs_hotkey_t *key)
	{
		auto registerer_type = obs_hotkey_get_registerer_type(key);

		if (registerer_type != OBS_HOTKEY_REGISTERER_SOURCE)
			return true;

		using namespace std;
		OBSBasicProperties *window =
				reinterpret_cast<OBSBasicProperties*>(data);
		void *registerer  = obs_hotkey_get_registerer(key);
		obs_weak_source_t *weak_source =
				static_cast<obs_weak_source_t*>(registerer);
		obs_source_t *source = OBSGetStrongRef(weak_source);

		if (!source)
			return true;

		const char *name = obs_source_get_name(source);
		const char *parent_name =
				obs_source_get_name(window->GetSource());

		if (strcmp(parent_name, name) != 0)
			return true;

		auto *label = new OBSHotkeyLabel;

		obs_hotkey_id partner = obs_hotkey_get_pair_partner_id(key);
		if (partner != OBS_INVALID_HOTKEY_ID) {
			window->pairLabels.emplace(obs_hotkey_get_id(key),
					make_pair(partner, label));
			window->pairIds.push_back(obs_hotkey_get_id(key));
		}

		OBSHotkeyWidget *hw = nullptr;

		auto combos = window->keys.find(id);
		if (combos == std::end(window->keys))
			hw = new OBSHotkeyWidget(id, obs_hotkey_get_name(key));
		else
			hw = new OBSHotkeyWidget(id, obs_hotkey_get_name(key),
				combos->second);

		label->setText(obs_hotkey_get_description(key));

		window->hotkeyLayout->setVerticalSpacing(0);
		window->hotkeyLayout->setFieldGrowthPolicy(
				QFormLayout::AllNonFixedFieldsGrow);
		window->hotkeyLayout->setLabelAlignment(
			Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

		window->hotkeyLayout->addRow(label, hw);

		connect(hw, &OBSHotkeyWidget::KeyChanged,
				window, &OBSBasicProperties::HotkeysChanged);

		window->hotkeys.emplace_back(false, hw);

		return true;
	};

	for (auto keyId : pairIds) {
		auto data1 = pairLabels.find(keyId);
		if (data1 == end(pairLabels))
			continue;

		auto &label1 = data1->second.second;
		if (label1->pairPartner)
			continue;

		auto data2 = pairLabels.find(data1->second.first);
		if (data2 == end(pairLabels))
			continue;

		auto &label2 = data2->second.second;
		if (label2->pairPartner)
			continue;

		QString tt = QTStr("Basic.Settings.Hotkeys.Pair");
		auto name1 = label1->text();
		auto name2 = label2->text();

		auto Update = [&](OBSHotkeyLabel *label, const QString &name,
				OBSHotkeyLabel *other, const QString &otherName)
		{
			label->setToolTip(tt.arg(otherName));
			label->setText(name + " *");
			label->pairPartner = other;
		};
		Update(label1, name1, label2, name2);
		Update(label2, name2, label1, name1);
	}

	obs_enum_hotkeys(SetHotkeys, this);
}

QWidget *OBSBasicProperties::HotkeysTab(QWidget *parent)
{
	QWidget *w = new QWidget(parent);
	LoadHotkeySettings();

	if (hotkeys.empty()) {
		QFormLayout *layout = new QFormLayout();
		layout->setLabelAlignment(Qt::AlignRight);
		QLabel *label = new QLabel(QTStr(
				"Basic.PropertiesWindow.NoHotkeys"));
		layout->addRow(label);
		w->setLayout(layout);
	} else {
		w->setLayout(hotkeyLayout);
	}

	return w;
}
