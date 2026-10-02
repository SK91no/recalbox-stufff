#include <utils/String.h>
#include <guis/GuiSelectDevice.h>
#include <WindowManager.h>
#include <Renderer.h>
#include <guis/GuiInputConfig.h>
#include <components/TextComponent.h>
#include <utils/locale/LocaleHelper.h>
#include <themes/MenuThemeData.h>
#include <themes/ThemeManager.h>

#include "components/MarkdownText.h"
#include "components/PictureGaleryComponent.h"

GuiSelectDevice::GuiSelectDevice(WindowManager& window, InputDevice& device, IDeviceConfigured* callbackInterface)
  : Gui(window)
  , mHoldTime(0)
  , mHolding(false)
  , mGrid(window, Vector2i(3, 7))
  , mDevice(device)
  , mCallback(callbackInterface)
{
 	const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	mColor = menuTheme.Text().color & 0xFFFFFF00;
	
	addChild(&mGrid);
	
	// title
	mTitle = std::make_shared<TextComponent>(mWindow, _("SELECT INPUT DEVICE TYPE"), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
	//mGrid.setEntry(mTitle, Vector2i(0, 0), false, true, Vector2i(1, 1), GridFlags::BORDER_BOTTOM);
	mGrid.setEntry(mTitle, Vector2i(1, 0), false, true);
	
	// device info
	String deviceInfo = (_F("for controler {0}") / mDevice.NameExtented()).ToString();
	mDeviceInfo = std::make_shared<TextComponent>(mWindow, deviceInfo, menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::Center);
	mGrid.setEntry(mDeviceInfo, Vector2i(1, 1), false, true);

	// message
	mGallery = std::make_shared<PictureGaleryComponent>(mWindow, ImgProps::NoCache | ImgProps::KeepRatio, this);
	mGallery->SetBackgroundColor(0xFFFFFF40);
	mGrid.setEntry(mGallery, Vector2i(1, 2), true, true);

	mPad = std::make_shared<TextScrollComponent>(mWindow, "", menuTheme.Text().font, menuTheme.Text().color, ::Alignment::TopCenter);
	mPad->SetBold(true);
	mGrid.setEntry(mPad, Vector2i(1, 3), false, true);

	String doc(_("Select your pad layout.\nUse any D-PAD or any joystick to select your controller type,\nthen **confirm by pressing any button**."));
  mDoc = std::make_shared<MarkdownText>(mWindow, doc, menuTheme.SmallText().color, ::Alignment::TopCenter);
	mDoc->SetPadding(Renderer::Instance().DisplayHeightAsFloat() * 0.02f);
	mGrid.setEntry(mDoc, Vector2i(1, 4), false, true);

	setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.75f , Renderer::Instance().DisplayHeightAsFloat() * 0.7f);
	setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

	// grid
	mGrid.setSize(mSize);
	mGrid.setColWidthPerc(0, 0.02f);
	mGrid.setColWidthPerc(1, 0);
	mGrid.setColWidthPerc(2, 0.02f);
	mGrid.setRowHeightPerc(0, ((float)mTitle->getFont()->Height() / mSize.y()) * 1.2f);
	mGrid.setRowHeightPerc(1, ((float)mDeviceInfo->getFont()->Height() / mSize.y()) * 1.6f);
	mGrid.setRowHeightPerc(2, 0.0f);
	mGrid.setRowHeightPerc(3, ((float)mPad->getFont()->Height() / mSize.y()) * 1.2f);
	mGrid.setRowHeightPerc(4, ((float)mDoc->Height() / mSize.y()) * 1.2f);
	mGrid.setRowHeightPerc(5, 0.015f);
	mGrid.setRowHeightPerc(6, 0.01f);

	// Inject images
	static struct { PadTypes::Type type; Path image; } sPadSerie[]
	{
		{ .type=PadTypes::HeightBitdo, .image=Path(":/pads/8bitdo.svg") },
		{ .type=PadTypes::XBox,        .image=Path(":/pads/xbox.svg") },
		{ .type=PadTypes::Playstation, .image=Path(":/pads/sony.svg") },
		{ .type=PadTypes::Arcade8,     .image=Path(":/pads/arcade8.svg") },
		{ .type=PadTypes::Arcade6,     .image=Path(":/pads/arcade6.svg") },
		{ .type=PadTypes::Arcade4,     .image=Path(":/pads/arcade4.svg") },
		// { .type=PadTypes::Gamecube,    .image=Path(":/pads/gc.svg") },
		// { .type=PadTypes::Megadrive6,  .image=Path(":/pads/megadrive6.svg") },
		// { .type=PadTypes::Megadrive3,  .image=Path(":/pads/megadrive3.svg") },
		// { .type=PadTypes::n64,         .image=Path(":/pads/n64.svg") },
	};
	for (const auto& pad : sPadSerie)
	  mGallery->SetImage((int)pad.type, pad.image);

	// Auto select — shared heuristic with InputDevice::LoadAutoConfiguration.
	if (PadTypes::Type detected = PadTypes::FromControllerName(mDevice.Name()); detected != PadTypes::__Count)
		mGallery->SetIndex((int)detected, false);
	PictureGalerySlotChanged(mGallery->Index());
}

void GuiSelectDevice::PictureGalerySlotChanged(int slot)
{
	String padName;
  switch ((PadTypes::Type)slot)
  {
	  case PadTypes::HeightBitdo: padName = _("8BitDo style controller (B down, A right)"); break;
	  case PadTypes::XBox: padName = _("XBox style controller (A down, B right)"); break;
	  case PadTypes::Playstation: padName = _("Playstation style controller"); break;
	  case PadTypes::Arcade8: padName = _("8 buttons Arcade controller"); break;
	  case PadTypes::Arcade6: padName = _("6 buttons Arcade controller"); break;
	  case PadTypes::Arcade4: padName = _("4 buttons Arcade controller"); break;
	  case PadTypes::Gamecube: padName = _("Gamecube style controller"); break;
	  case PadTypes::Megadrive6: padName = _("6 buttons Megadrive/Genesis controller"); break;
	  case PadTypes::Megadrive3: padName = _("3 buttons Megadrive/Genesis controller"); break;
	  case PadTypes::n64: padName = _("Nintendo 64 style controller"); break;
	  case PadTypes::__Count:
		default: break;
  }
	if (!padName.empty())
		mPad->setText(padName);
}

bool GuiSelectDevice::ProcessInput(const InputCompactEvent& event)
{
  if (event.KeyCode() == SDLK_ESCAPE || event.HotkeyPressed())
  {
    // cancel configuring
    Close();
    return true;
  }

  if ((&event.Device() == &mDevice))
  {
  	if (event.RawEvent().AnyButtonPressed() && !event.AnyDirectionPressed())
  	{
  		// started holding
  		mHoldTime = sHoldingTime;
  		mHolding = true;
  	}
  	else if (event.RawEvent().AnyButtonReleased() && !event.AnyDirectionReleased())
  	{
  		if (!event.Device().IsConfigured() && mHoldTime < 0) mGallery->MoveRelative(((event.RawEvent().Id() & 1) * 2) - 1);
  		else if (mHoldTime > 0)
  		{
  			mWindow.pushGui(new GuiInputConfig(mWindow, &mDevice, (PadTypes::Type)mGallery->Index(), nullptr));
  			Close();
  		}
  		mHoldTime = 0;
  		mHolding = false;
  	}
  }

  return mGallery->ProcessInput(event);
}

void GuiSelectDevice::Update(int deltaTime)
{
	if(mHolding) mHoldTime -= deltaTime;

	Component::Update(deltaTime);
}

void GuiSelectDevice::Render(const Transform4x4f& transform)
{
	Gui::Render(transform);

	if (mHolding)
	{
		if (int length = (sHoldingTime - mHoldTime); length > sHoldingDeadTime)
		{
			if (length > sHoldingTime) length = sHoldingTime;
      Transform4x4f trans = transform * getTransform();
			Renderer::SetMatrix(trans);
			int y = (int)mGrid.getRowHeight(0, 4);
			int w = (int)mGrid.getColWidth(1) * length / sHoldingTime;
			Rectangle area((int)mGrid.getColWidth(0), y, w, (int)mGrid.getRowHeight(5));
			Renderer::DrawRectangle(area, 0x00C00080);
		}
	}
}
