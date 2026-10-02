#include <utils/String.h>
#include <guis/GuiDetectDevice.h>
#include <WindowManager.h>
#include <Renderer.h>
#include <guis/GuiInputConfig.h>
#include <components/TextComponent.h>
#include <utils/locale/LocaleHelper.h>
#include <themes/MenuThemeData.h>
#include <themes/ThemeManager.h>

#include "GuiSelectDevice.h"

#define HOLD_TIME 1000

GuiDetectDevice::GuiDetectDevice(WindowManager& window, bool firstRun, IDeviceConfigured* callback, bool testOnly)
  : Gui(window),
    mFirstRun(firstRun),
    mTestOnly(testOnly),
    mHoldingConfig(nullptr),
    mHoldTime(0),
    mGrid(window, Vector2i(1, 5)),
    mCallback(callback)
{

 	const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	mColor = menuTheme.Text().color & 0xFFFFFF00;
	
	addChild(&mGrid);
	
	// title
	mTitle = std::make_shared<TextComponent>(mWindow, firstRun ? _("WELCOME") : _("CONFIGURE INPUT"), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);
	
	// device info
	String deviceInfo;
	int numDevices = InputManager::Instance().ConfigurableDeviceCount();
	
	if(numDevices > 0) deviceInfo = _N("%i GAMEPAD DETECTED", "%i GAMEPADS DETECTED", numDevices).Replace("%i", String(numDevices));
	else               deviceInfo = _("NO GAMEPADS DETECTED");
	mDeviceInfo = std::make_shared<TextComponent>(mWindow, deviceInfo, menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::Center);
	mGrid.setEntry(mDeviceInfo, Vector2i(0, 1), false, true);

	// message
	mMsg1 = std::make_shared<TextComponent>(mWindow, _("HOLD A BUTTON ON YOUR DEVICE TO CONFIGURE IT."), menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::Center);
	mGrid.setEntry(mMsg1, Vector2i(0, 2), false, true);

  mMsg2 = std::make_shared<TextComponent>(mWindow, _("PRESS ESC OR THE HOTKEY TO CANCEL."), menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::Center);
	mGrid.setEntry(mMsg2, Vector2i(0, 3), false, true);

	// currently held device
	mDeviceHeld = std::make_shared<TextComponent>(mWindow, "", menuTheme.Text().font, mColor, ::Alignment::Center);
	mGrid.setEntry(mDeviceHeld, Vector2i(0, 4), false, true);

	float y= (((float)mMsg1->getFont()->Height() * 4.0f) + (float)mTitle->getFont()->Height() ) / Renderer::Instance().DisplayHeightAsFloat() + 0.08f;

	setSize(Renderer::Instance().Is480pOrLower() ? Renderer::Instance().DisplayWidthAsFloat() * 0.9f : Renderer::Instance().DisplayWidthAsFloat() * 0.6f, Renderer::Instance().DisplayHeightAsFloat() * y);
	setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);
}

void GuiDetectDevice::onSizeChanged()
{
	// grid
	mGrid.setSize(mSize);
	mGrid.setRowHeightPerc(0, (float)mTitle->getFont()->Height() / mSize.y());
	mGrid.setRowHeightPerc(1, (float)mDeviceInfo->getFont()->Height() / mSize.y());
	mGrid.setRowHeightPerc(2, (float)mMsg1->getFont()->Height() / mSize.y());
	mGrid.setRowHeightPerc(3, (float)mMsg2->getFont()->Height() / mSize.y());
	mGrid.setRowHeightPerc(4, (float)mDeviceHeld->getFont()->Height() *1.5f / mSize.y());
}


bool GuiDetectDevice::ProcessInput(const InputCompactEvent& event)
{
  if (!mFirstRun && (event.KeyCode() == SDLK_ESCAPE || event.HotkeyPressed()))
  {
    // cancel configuring
    Close();
    return true;
  }

  if (event.Device().IsConfigurable())
  {
    if (event.RawEvent().AnyButtonPressed() && mHoldingConfig == nullptr)
    {
      // started holding
      mHoldingConfig = &event.Device();
      mHoldTime = HOLD_TIME;
      mDeviceHeld->setText(event.Device().Name().ToUpperCase());
    }
    else if (event.RawEvent().AnyButtonReleased() && mHoldingConfig == &event.Device())
    {
      // cancel
      mHoldingConfig = nullptr;
      mDeviceHeld->setText("");
      mAlpha = 0;
    }
  }

	return true;
}

void GuiDetectDevice::Update(int deltaTime)
{
	if(mHoldingConfig != nullptr)
	{
		mHoldTime -= deltaTime;
		// Fade in over HOLD_TIME, in a single division by the constant. The previous two-step form
		// raised SIGFPE twice over: deltaTime can be 0 on a frame shorter than the 1ms tick
		// resolution, and the resulting frame count itself reaches 0 once deltaTime exceeds HOLD_TIME.
		mAlpha += (255 * deltaTime) / HOLD_TIME;
		mDeviceHeld->setColor(mColor | mAlpha);
		if(mHoldTime <= 0)
		{
			// picked one!
			if (mTestOnly)
				// Skip GuiSelectDevice (no need to pick a pad image): reuse the device's
				// stored PadType and jump straight into the read-only test view.
				mWindow.pushGui(new GuiInputConfig(mWindow, mHoldingConfig, mHoldingConfig->PadType(), mCallback, true));
			else
				mWindow.pushGui(new GuiSelectDevice(mWindow, *mHoldingConfig, mCallback));
			Close();
		}
	}
}
