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

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPointer>
#include <QSplitter>
#include <QTabBar>
#include "qt-display.hpp"
#include <obs.hpp>
#include "window-basic-filters.hpp"
#include "window-basic-transform.hpp"
#include "hotkey-edit.hpp"

enum PropertiesType {
	Scene,
	Source,
	Transition
};

class OBSPropertiesView;
class OBSBasic;

class OBSBasicProperties : public QDialog {
	Q_OBJECT

private:
	QPointer<OBSQTDisplay> preview;

	OBSBasic            *main;
	bool                acceptClicked;
	bool                hotkeysChanged;
	enum PropertiesType propType;

	OBSSource  source;
	OBSSceneItem item;
	OBSSignal  removedSignal;
	OBSSignal  renamedSignal;
	OBSSignal  updatePropertiesSignal;
	OBSData    oldSettings;
	OBSPropertiesView *view;
	OBSBasicFilters *filters;
	OBSBasicTransform *transformWindow;
	QDialogButtonBox *buttonBox;
	QSplitter *windowSplitter;
	QTabBar *tabsLeft;
	QTabBar *tabsRight;
	QWidget *HotkeysTab(QWidget *parent);
	QWidget *PerSceneTransitionWidget(QWidget *parent);
	QWidget *AdvancedItemTab(QWidget *parent);
	QWidget *AdvancedGlobalTab(QWidget *parent);
	QComboBox *combo;
	QSpinBox *duration;
	QComboBox *deinterlace;
	QComboBox *sf;
	QComboBox *order;
	QWidget *hotkeysTab;
	QWidget *trOverride;
	QWidget *itemAdvanced;
	QWidget *globalAdvanced;

	static void SourceRemoved(void *data, calldata_t *params);
	static void SourceRenamed(void *data, calldata_t *params);
	static void UpdateProperties(void *data, calldata_t *params);
	static void DrawPreview(void *data, uint32_t cx, uint32_t cy);
	bool ConfirmQuit();
	int  CheckSettings();
	void Cleanup();

	std::vector<std::pair<bool, QPointer<OBSHotkeyWidget>>> hotkeys;
	std::vector<obs_hotkey_id> pairIds;
	std::map<obs_hotkey_id,
			std::pair<obs_hotkey_id, OBSHotkeyLabel*>> pairLabels;
	std::map<obs_hotkey_id, std::vector<obs_key_combination_t>> keys;

	OBSSignal hotkeyRegistered;
	OBSSignal hotkeyUnregistered;

	void LoadHotkeySettings(obs_hotkey_id ignoreKey=OBS_INVALID_HOTKEY_ID);

	void ShowGeneral();
	void ShowFilters();
	void ShowHotkeys();
	void ShowTransform();
	void ShowGlobalAdvanced();
	void ShowItemAdvanced();
	void ShowPerSceneTransition();
	void HideLeft();
	void HideRight();

	void LoadOriginalSettings();
	void SaveOriginalSettings();

	void SetHotkeyDefaults();

	void ResetSourcesDialog();
	void ResetScenesDialog();
	void ResetTransitionsDialog();

	obs_transform_info originalTransform;
	obs_sceneitem_crop originalCrop;

	obs_deinterlace_mode originalDeinterlaceMode;
	obs_deinterlace_field_order originalDeinterlaceOrder;
	obs_scale_type originalScaleFilter;

	const char *originalTransition;
	int originalDuration;

private slots:
	void on_buttonBox_clicked(QAbstractButton *button);
	void SetDeinterlacingMode(int index);
	void SetDeinterlacingOrder(int index);
	void SetScaleFilter(int index);
	void SaveHotkeySettings();
	void HotkeysChanged();
	void ReloadHotkeys(obs_hotkey_id ignoreKey);
	void TabsLeftClicked(int index);
	void TabsRightClicked(int index);

public:
	OBSBasicProperties(QWidget *parent, OBSSource source_,
			PropertiesType type);
	~OBSBasicProperties();

	void Init();
	void OpenTransformTab();

	QFormLayout *hotkeyLayout;

	OBSSource GetSource();

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual void reject() override;
};
