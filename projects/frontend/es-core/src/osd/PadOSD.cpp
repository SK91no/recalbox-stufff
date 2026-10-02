//
// Created by bkg2k on 02/10/23.
//

#include "PadOSD.h"
#include <input/InputManager.h>
#include <rendering/fonts/FontManager.h>
#include <Renderer.h>

PadOSD::PadOSD(WindowManager& window, Side side)
  : BaseOSD(window, side, true)
  , mMapper(InputManager::Instance().Mapper())
  , mFont(&FontManager::Instance().FromDefault(1.f / 32.f, false))
  , mAlpha { sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha, sMinAlpha }
  , mPadChar(0)
  , mPadCount(0)
  , mActive(false)
  , mForcedActive(false)
{
  // Subscription is automatic through the IPadOSDNotification base constructor.
  UpdatePadIcon();
  UpdateActiveFlag();
}

void PadOSD::UpdatePadIcon()
{
  mPadChar = 0xF25E;
  switch(RecalboxConf::Instance().GetPadOSDType())
  {
    case RecalboxConf::PadOSDType::MD: mPadChar = 0xF26C; break;
    case RecalboxConf::PadOSDType::XBox: mPadChar = 0xF2F0; break;
    case RecalboxConf::PadOSDType::PSX: mPadChar = 0xF2C8; break;
    case RecalboxConf::PadOSDType::N64: mPadChar = 0xF260; break;
    case RecalboxConf::PadOSDType::DC: mPadChar = 0xF26E; break;
    case RecalboxConf::PadOSDType::Snes:
    default: break;
  }
}

bool PadOSD::ProcessInput(const InputCompactEvent& event)
{
  // Pad alpha
  if (event.Device().IsPad())
    if (event.AnythingPressed() || event.AnythingReleased())
      if (int padIndex = InputManager::Instance().Mapper().PadIndexFromDeviceIdentifier(event.RawEvent().Device()); padIndex >= 0)
        mAlpha[padIndex] = sMaxAlpha;
  return false;
}

void PadOSD::Update(int deltaTime)
{
  // Update pads
  if (!mActive && !mForcedActive) return;
  // Set alphas
  mPadCount = InputManager::Instance().Mapper().ConnectedPadCount();
  for(int i = Input::sMaxInputDevices; --i >= 0; )
    mAlpha[i] = Math::clampi(mAlpha[i] -= deltaTime / 4, i < mPadCount ? sMinAlpha : 0, sMaxAlpha);
}

void PadOSD::Render(const Transform4x4f& transform)
{
  (void)transform;

  static int flashing = 0;
  InputManager& inputManager = InputManager::Instance();
  int step = Renderer::Instance().DisplayHeightAsInt() / 32;
  int w = (int) mFont->CharWidth(mPadChar) + Renderer::Instance().DisplayWidthAsInt() / 160;
  int bh = 0;
  const InputMapper::PadList padList = mMapper.GetPads();
  for (int i = mPadCount; --i >= 0;)
  {
    int y = i * (step + 1);
    mFont->DrawChar(mPadChar, 0, y, sColor | mAlpha[i], ::Alignment::CenterLeft);
    InputDevice& device = inputManager.GetDeviceConfigurationFromIndex(padList[i].mIndex);
    if (device.HasBatteryLevel() && (device.BatteryLevel() > 15 || ((flashing >> 3) & 3) != 0))
    {
      float hr = (float) bh / (float) step;
      FontEffect effect;
      effect.SetScaling(hr, hr);
      mFont->DrawChar(device.BatteryLevelIcon(), w, y, sColor | mAlpha[i], ::Alignment::CenterLeft, effect);
    }
  }
  flashing++;
}

int PadOSD::OSDAreaWidth() const
{
  return 2 * (int) mFont->CharWidth(mPadChar);
}

int PadOSD::OSDAreaHeight() const
{
  return ((Renderer::Instance().DisplayHeightAsInt() / 32) + 1) * mPadCount;
}

void PadOSD::UpdateActiveFlag()
{
  mActive = RecalboxConf::Instance().GetPadOSD() || mForcedActive;
}
