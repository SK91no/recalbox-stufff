//
// Created by bkg2k on 21/11/2019.
//

#include <Renderer.h>
#include <utils/locale/LocaleHelper.h>
#include "SplashView.h"
#include <rendering/fonts/FontManager.h>
#include <hardware/Board.h>
#include "hardware/cardreader/CardReader.h"

SplashView::SplashView(WindowManager& window)
  : Gui(window)
  , mLogo(window, ImgProps::KeepRatio | ImgProps::ImmediateLoad)
  , mLoading(window, _("LOADING..."), Board::Instance().CrtBoard().GetCrtAdapter() != CrtAdapterType::None ? &FontManager::Instance().FromFile(Path(FONT_PATH_CRT),
    7 * Math::ceil(Renderer::Instance().IsRotatedSide() ? Renderer::Instance().DisplayWidthAsFloat() / 288.f : Renderer::Instance().DisplayHeightAsFloat() / 288.f ), true) : &FontManager::Instance().FromDefault( FONT_SIZE_MEDIUM, false), 0)
  , mSystemCount(0)
  , mSystemLoaded(0)
{
  mIsRecalboxRGBHat = Board::Instance().CrtBoard().IsCrtAdapterAttached();


  mIsDarkMode = RecalboxConf::Instance().GetDarkMode();
  //Color as variables for easier follow up
  mDarkModeBgColor = 0x35495EFF;
  mDarkModeTextColor = 0xFFFFFFFF;
  mDarkModeLoadingBarColor = 0x9F9F9F9F;
  mDarkModeLoadingBarBgColor = 0xFFFFFFFF;

  bool isRGBJamma = Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma;
  bool isRGBDual2 = Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBDual2;
  bool isJVS = Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::JVS;
  mPosition.Set(0,0,0);
  mSize.Set(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());

  //mLogo.setOrigin(0.5f, 0.5f);
  if (CardReader::Instance().ConsoleMode())
  {
    mLogo.setResize(0.f, Renderer::Instance().DisplayHeightAsFloat() * 0.25f);
    mLogo.setImage(Path(":/cardreader/recalcards_logo.svg"), false, true);
    mLogo.setOrigin(0.5f,0.5f);
    mLogo.setPosition(0.5 * Renderer::Instance().DisplayWidthAsFloat(), 0.5 * Renderer::Instance().DisplayHeightAsFloat());
    mLogo.SetColor(0xffffffd0);
  }
  else
  {
    mLogo.setResize(0.f, Renderer::Instance().DisplayHeightAsFloat() * (mIsRecalboxRGBHat ? (isJVS ? 0.4f : (isRGBJamma ? 0.18f : 0.7f)) : 0.5f));
    mLogo.setImage(Path(mIsRecalboxRGBHat ? (isJVS ? ":/crt/logojvs.svg" : (isRGBJamma ? ":/crt/logojamma.svg" : (isRGBDual2 ? ":/crt/logorgbdual2.png" : ":/crt/logorgbdual1.png"))) : (mIsDarkMode ? ":/Recalbox_logo_White.svg" : ":/Recalbox_logo_Grey.svg")), false, true);
    mLogo.setPosition((Renderer::Instance().DisplayWidthAsFloat() - mLogo.getSize().x()) / 2,
                     (Renderer::Instance().DisplayHeightAsFloat() - mLogo.getSize().y()) / ( isRGBJamma ? 2.f : 2.5f));
  }


  mLoading.setHorizontalAlignment(::HorizontalAlignment::Center);
  mLoading.setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat() / 10.0f);
  mLoading.setPosition(0.0f, Renderer::Instance().DisplayHeightAsFloat() * 0.8f, 0.0f);
  mLoading.setColor(mIsRecalboxRGBHat ? 0x9A9A9AFF : (mIsDarkMode ? mDarkModeTextColor : 0x656565FF));
  mBgColor = CardReader::Instance().ConsoleMode() ? CardReader::Instance().ConsoleModeBackgroundColor() : (mIsRecalboxRGBHat ? 0x35495EFF : (mIsDarkMode ? mDarkModeBgColor : 0xFFFFFFFF));
}

void SplashView::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = (parentTrans * getTransform()).round();
  Renderer::SetMatrix(trans);

  Renderer::DrawRectangle(0, 0, Renderer::Instance().DisplayWidthAsInt(), Renderer::Instance().DisplayHeightAsInt(), mBgColor);

  int w = (int)(Renderer::Instance().DisplayWidthAsFloat() / (Renderer::Instance().Is480pOrLower() ? 5.0f : 10.0f));
  int h = (int)(Renderer::Instance().DisplayHeightAsFloat() / (Renderer::Instance().Is480pOrLower() ? 70.0f : 80.0f));
  int x = (int)((Renderer::Instance().DisplayWidthAsFloat() - (float)w) / 2.0f);
  int y = (int)(Renderer::Instance().DisplayHeightAsFloat() * 0.9f);
  Renderer::DrawRectangle(x, y, w, h, mIsRecalboxRGBHat ? 0x404040FF : (mIsDarkMode ? mDarkModeLoadingBarBgColor : 0xC0C0C0FF));
  if (mSystemCount != 0)
  {
    int max = Math::clampi(mSystemCount, 1, INT32_MAX);
    int val = Math::clampi(mSystemLoaded, 0, max);
    w = (w * val) / max;
    Renderer::DrawRectangle(x, y, w, h, mIsRecalboxRGBHat ? 0xA0A0A0FF : (mIsDarkMode ? mDarkModeLoadingBarColor : 0x606060FF));
  }

  mLogo.Render(trans);
  mLoading.Render(trans);

  Component::Render(trans);
}

void SplashView::SetMaximum(int maximum)
{
  mSystemCount = maximum;
  mWindow.DoWake(); // Kill screensaver
  mWindow.RenderAll();
}

void SplashView::SetProgress(int value)
{
  mSystemLoaded = value;
  mWindow.DoWake(); // Kill screensaver
  mWindow.RenderAll();
}

void SplashView::Increment()
{
  if (++mSystemLoaded >= mSystemCount) mSystemLoaded = mSystemCount - 1;
  mWindow.DoWake(); // Kill screensaver
  mWindow.RenderAll();
}

void SplashView::Quit()
{
  mSystemLoaded = mSystemCount = 0;
  mLoading.setText(_("PLEASE WAIT..."));
}

void SplashView::SystemLoadingPhase(ISystemLoadingPhase::Phase phase)
{
  switch(phase)
  {
    case Phase::RegularSystems: mLoading.setText(_("LOADING SYSTEMS...")); break;
    case Phase::VirtualSystems: mLoading.setText(_("LOADING VIRTUAL SYSTEMS...")); break;
    case Phase::Completed: mLoading.setText(_("INITIALIZING INTERFACE...")); break;
    default: break;
  }
}

bool SplashView::CollectHelpItems(Help& help)
{
  help.Clear();
  return true;
}

