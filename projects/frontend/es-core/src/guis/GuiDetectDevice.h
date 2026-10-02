#pragma once

#include <guis/Gui.h>
#include <components/NinePatchComponent.h>
#include <components/ComponentGrid.h>
#include <input/InputDevice.h>

#include "IDeviceConfigured.h"

class TextComponent;

class GuiDetectDevice : public Gui
{
public:
	/*!
	 * @param testOnly When true, on detection the picked device is sent directly
	 * to GuiInputConfig in test-only mode (no GuiSelectDevice prompt for the
	 * pad image type — the device's stored PadType is reused).
	 */
	GuiDetectDevice(WindowManager& window, bool firstRun, IDeviceConfigured* callback, bool testOnly = false);

	bool ProcessInput(const InputCompactEvent& event) override;
	void Update(int deltaTime) override;
	void onSizeChanged() override;

private:
	bool mFirstRun;
	bool mTestOnly;
	InputDevice* mHoldingConfig;
	int mHoldTime;

	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextComponent> mMsg1;
	std::shared_ptr<TextComponent> mMsg2;
	std::shared_ptr<TextComponent> mDeviceInfo;
	std::shared_ptr<TextComponent> mDeviceHeld;
	unsigned int mColor;
	unsigned int mAlpha = 0;

	IDeviceConfigured* mCallback;
};
