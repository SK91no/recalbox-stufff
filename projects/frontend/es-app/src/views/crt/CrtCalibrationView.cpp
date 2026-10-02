//
// Created by bkg2k on 21/11/2019.
//

#include <Renderer.h>
#include <utils/locale/LocaleHelper.h>
#include "CrtCalibrationView.h"
#include "views/ViewController.h"
//#include "utils/Files.h"
#include "sdl2/Sdl2Init.h"
//#include "themes/MenuThemeData.h"
#include "recalbox/RecalboxSystem.h"
//#include "guis/GuiMsgBox.h"

//#define FONT_SIZE_LOADING ((unsigned int)(0.065f * Math::min(Renderer::Instance().DisplayHeightAsFloat(), Renderer::Instance().DisplayWidthAsFloat())))

CrtCalibrationView::CrtCalibrationView(WindowManager& window)
  : Gui(window)
  , mPattern(window, ImgProps::None)
  , mGrid(window, Vector2i(1, 3))
  , mEvent(*this)
  , mSequence(nullptr)
  , mSequenceIndex(0)
  , mOriginalVOffset(0)
  , mOriginalHOffset(0)
{
}

void CrtCalibrationView::Initialize(CalibrationType calibrationType)
{
  mSequenceIndex = 0;
  switch(calibrationType){
    case kHz31:
      mSequence = sForced31khz; break;
    case kHz15_50Hz:
      mSequence = sPALOnly; break;
    case kHz15_60Hz:
      mSequence = sNTSCOnly; break;
    case kHz15_60Hz_plus_kHz31:
      mSequence = sMultiSync; break;
    case kHz31_no_120:
      mSequence = s31khzNo120; break;
    case kHz15_60plus50Hz:
    default:
      mSequence = sPALNTSC; break;
  }

  SetResolution(mSequence[mSequenceIndex]);
  Initialize();
  UpdateViewport();
}

void CrtCalibrationView::Initialize()
{
  mOriginalVOffset = CrtConf::Instance().GetCrtModeOffsetVerticalOffset(mSequence[mSequenceIndex]);
  mOriginalHOffset = CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(mSequence[mSequenceIndex]);
  { LOG(LogDebug) << "[CrtCalibrationView] Initialize: mSequenceIndex= " << mSequenceIndex << ", resolution= " << (int)(mSequence[mSequenceIndex]); }

  mPosition.Set(0,0,0);
  mSize.Set(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());

  mGrid.ClearEntries();
  mGrid.setSize(Renderer::Instance().DisplayWidthAsFloat() * (320.0f / 768.f), Renderer::Instance().DisplayHeightAsFloat() * (156.f / 576.f));
  mGrid.setPosition(Renderer::Instance().DisplayWidthAsFloat() * (120.f / 768.f), Renderer::Instance().DisplayHeightAsFloat() * (100.0f / 576.f));
  addChild(&mGrid);

  mPattern.setResize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
  mPattern.setOrigin(.5f, .5f);
  mPattern.setPosition(Renderer::Instance().DisplayWidthAsFloat() / 2.f, Renderer::Instance().DisplayHeightAsFloat() / 2.f, .0f);

  NewFont* font = &FontManager::Instance().FromFile(Path(FONT_PATH_CRT), 7 , false);
  mHorizontalOffsetText = std::make_shared<TextComponent>(mWindow, "H OFFSET", font, 0xFFFFFFFF);
  mVerticalOffsetText = std::make_shared<TextComponent>(mWindow, "V OFFSET", font, 0xFFFFFFFF);

  mGrid.setEntry(mHorizontalOffsetText, { 0, 0 }, false);
  mGrid.setEntry(mVerticalOffsetText, { 0, 1 }, false);
}

void CrtCalibrationView::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = (parentTrans * getTransform()).round();
  Renderer::SetMatrix(trans);

  Renderer::DrawRectangle(0, 0, Renderer::Instance().DisplayWidthAsInt(), Renderer::Instance().DisplayHeightAsInt(), 0x000000FF);
  mPattern.Render(trans);
  Renderer::SetMatrix(trans);
  Component::Render(trans);

  // Wake up permanently
  mWindow.DoWake();
}

bool CrtCalibrationView::CollectHelpItems(Help& help)
{
  help.Set(HelpType::AllDirections, _("MOVE SCREEN"))
      .Set(Help::Valid(), _("NEXT RESOLUTION"))
      .Set(Help::Cancel(), _("QUIT"))
      .ForcePosition(0.05, 0.88, 0.9);
  return true;
}

void CrtCalibrationView::SetResolution(CrtResolution resolution)
{
  WindowManager::Finalize();
  Sdl2Init::Finalize();
  Sdl2Init::Initialize();
  int w = 0;
  int h = 0;
  switch(resolution)
  {
    case CrtResolution::r224p: w = 1920; h = 224; break;
    case CrtResolution::r240p: w = 1920; h = 240; break;
    case CrtResolution::r320x240p: w = 320; h = 240; break;
    case CrtResolution::r288p: w = 1920; h = 288; break;
    case CrtResolution::r384x288p: w = 384; h = 288; break;
    case CrtResolution::r480i: w = 640; h = 480; break;
    case CrtResolution::r576i: w = 768; h = 576; break;
    case CrtResolution::r480p: w = 640; h = 480; break;
    case CrtResolution::r240p120Hz: w = 1920; h = 240; break;
    case CrtResolution::r1920x480p: w = 1920; h = 480; break;
    case CrtResolution::_rCount:
    case CrtResolution::rNone: // Original resolution
    default: break;
  }

  { LOG(LogDebug) << "[CrtCalibrationView] SetResolution: set resolution to " << w << "x" << h; }

  mWindow.Initialize(w, h);
  mWindow.normalizeNextUpdate();
  mWindow.UpdateHelpSystem();
  mPattern.setResize(0.0f, Renderer::Instance().DisplayHeightAsFloat());
  mPattern.setImage(Path(":/crt/" + CrtConf::CrtResolutionFromEnum(resolution) + ".png"), false);
  mPattern.setOrigin(.5f, .5f);
  mPattern.setPosition(Renderer::Instance().DisplayWidthAsFloat() / 2.f, Renderer::Instance().DisplayHeightAsFloat() / 2.f, .0f);
  InputManager::Instance().Refresh(&mWindow, false);
}


static const int porch[CrtResolution::_rCount][Offset::Count]
{
  // { -1 * hback / ratio (max 20), hfront_porch / ratio, Up , Down }
  {0,0,0,0}, // rNone, //!< Null element
  {-20,12,-15,10}, // r224p, //!< PAL + NTSC 224p
  {-20,12,-16,1}, // r240p, //!< PAL + NTSC 224p
  {-20,12,-16,4}, // r288p, //!< PAL only
  {-15,4,-16,3}, // r480i, //!< NTSC interlaced hires
  {-12,4,-12,6}, // r576i, //!< PAL interlaced hires
  {-20,12,-10,10}, // r480p, //!< 31kHz
  {-20,8,-14,4}, // r240p120Hz, //!< 31kHz 240
  {-46,4,-14,4}, // r320x240p, //!< 320x240
  {-40,16,-12,3}, // r384x288p, //!< 384x288
  {-20,8,-10,10}, // r1920x480p
};

bool CrtCalibrationView::ProcessInput(const InputCompactEvent& event)
{
  CrtResolution reso = mSequence[mSequenceIndex];
  bool update = false;
  if (event.CancelReleased()) mEvent.Send(); // Synchronous quit (delete this class)
  else if (event.ValidReleased()) // Validate: go to next screen
  {
    CrtConf::Instance().Save();
    reso = mSequence[++mSequenceIndex];
    { LOG(LogDebug) << "[CrtCalibrationView] ProcessInput: selected resolution " << (int)mSequence[mSequenceIndex] << " at index " << mSequenceIndex; }
    // Skip interlaced if needed
    if(!Board::Instance().CrtBoard().HasInterlacedSupport())
      while(mSequence[mSequenceIndex] == CrtResolution::r480i || mSequence[mSequenceIndex] == CrtResolution::r576i)
      {
        { LOG(LogDebug) << "[CrtCalibrationView] ProcessInput: skipping resolution " << (int)mSequence[mSequenceIndex] << " at index " << mSequenceIndex; }
        reso = mSequence[++mSequenceIndex];
      }
    if (mSequence[mSequenceIndex] == CrtResolution::rNone)
      mEvent.Send();
    else {
      update = true;
    }
  }
  else if (event.AnyUpReleased())
  {
    if(CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso) > porch[reso][Offset::Up])
    {
      CrtConf::Instance().SetCrtModeOffsetVerticalOffset(reso, CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso) - 1);
      update = true;
    }
  }
  else if (event.AnyDownReleased())
  {
    if(CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso) < porch[reso][Offset::Down])
    {
      CrtConf::Instance().SetCrtModeOffsetVerticalOffset(reso, CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso) + 1);
      update = true;
    }
  }
  else if (event.AnyLeftReleased())
  {
    if(CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso) > porch[reso][Offset::Left])
    {
      CrtConf::Instance().SetCrtModeOffsetHorizontalOffset(reso, CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso) - 1);
      update = true;
    }
  }
  else if (event.AnyRightReleased())
  {
    if(CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso) < porch[reso][Offset::Right])
    {
      CrtConf::Instance().SetCrtModeOffsetHorizontalOffset(reso, CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso) + 1);
      update = true;
    }
  }
  if(update)
  {
    CrtConf::Instance().Save();
    SetResolution(reso);
    Initialize();
    UpdateViewport();
  }
  return true;
}

void CrtCalibrationView::ReceiveSyncMessage()
{
  SetResolution(CrtResolution::rNone);
  ViewController::Instance().BackToPreviousView();
}

void CrtCalibrationView::UpdateViewport()
{
  CrtResolution reso = mSequence[mSequenceIndex];

  // Reference
  int reference = ((Renderer::Instance().DisplayWidthAsInt()) * 1840) / 1920;

  mPattern.setSize((float) (reference + CrtConf::Instance().GetCrtViewportWidth(reso)), mPattern.getSize().y());
  mPattern.setPosition(Renderer::Instance().DisplayWidthAsFloat() / 2.f, Renderer::Instance().DisplayHeightAsFloat() / 2.f, .0f);

  String minMax = "";
  if(porch[reso][Offset::Left] >= CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso)) minMax.Append("(MIN)");
  if(porch[reso][Offset::Right] <= CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso)) minMax.Append("(MAX)");
  mHorizontalOffsetText->setText(
      _("Horizontal offset:").Append(' ').Append(CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso)).Append(minMax));
  minMax = "";
  if(porch[reso][Offset::Up] >= CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso)) minMax.Append("(MIN)");
  if(porch[reso][Offset::Down] <= CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso)) minMax.Append("(MAX)");
  mVerticalOffsetText->setText(
      _("Vertical offset:").Append(' ').Append(CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso)).Append(minMax));

  if (mOriginalHOffset != CrtConf::Instance().GetCrtModeOffsetHorizontalOffset(reso)) {
    mHorizontalOffsetText->setColor(0xAAAAFFFF);
  } else {
    mHorizontalOffsetText->setColor(0xFFFFFFFF);
  }
  if (mOriginalVOffset != CrtConf::Instance().GetCrtModeOffsetVerticalOffset(reso)) {
    mVerticalOffsetText->setColor(0xAAAAFFFF);
  } else {
    mVerticalOffsetText->setColor(0xFFFFFFFF);
  }
}
