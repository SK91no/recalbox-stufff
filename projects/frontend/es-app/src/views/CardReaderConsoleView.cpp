//
// Created by digi on 12/22/25.
//

#include "CardReaderConsoleView.h"
#include <filesystem>
//#include "VideoEngine.h"
#include "ViewController.h"
#include "hardware/cardreader/CardReader.h"


CardReaderConsoleView::CardReaderConsoleView(WindowManager& window)
  : Gui(window), mVideo(window, nullptr, VideoComponent::AllowedEffects::None), started(false)
{
  mPosition.Set(0,0,0);
  mIsProcessing = true;
}

void CardReaderConsoleView::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = (parentTrans * getTransform()).round();
  Renderer::SetMatrix(trans);
  Renderer::DrawRectangle(.0, .0, mSize.x(), mSize.y(), Colors::Common::Blue);
  if (!started)
    Reset();
  mVideo.Render(parentTrans);
}

bool CardReaderConsoleView::exit()
{
  AudioScheduler::Instance().StopAll();
  ViewController::Instance().goToStart();
  return true;
}

bool CardReaderConsoleView::ProcessInput(const InputCompactEvent& event)
{
  if(RecalboxConf::Instance().GetCRConsoleModeExit() == "hkstart" && event.HotkeyStartReleased())
    return exit();
  if (event.StartPressed())
    mStartPressed = DateTime();
  else if (RecalboxConf::Instance().GetCRConsoleModeExit() == "start5sec" &&
    event.StartReleased() && DateTime().ToEpochTime() - mStartPressed .ToEpochTime() > 4)
  {
    return exit();
  }
  return false;
}

void CardReaderConsoleView::Reset()
{
  mVideo.setOrigin(0.5, 0.5);
  mVideo.setPosition(0.5 * Renderer::Instance().DisplayWidthAsFloat(),
                     0.5 * Renderer::Instance().DisplayHeightAsFloat());
  mVideo.setKeepRatio(true);
  mVideo.setResize(Renderer::Instance().DisplayWidthAsFloat() * 10, Renderer::Instance().DisplayHeightAsFloat());

  Path video = CardReader::Instance().ConsoleModeVideo();
  if (video != Path::Empty)
  {
    mVideo.setVideo(video, 0, 0, false);
    mVideo.OnPathChanged();
    Path sound = CardReader::Instance().ConsoleModeSound();
    if (sound != Path::Empty)
    {
      { LOG(LogDebug) << "[CardReaderConsoleView] Starting Bios sound"; }
      AudioScheduler::Instance().PlayMusicFile(sound, true);
    }
    else
      AudioScheduler::Instance().PlayNextMusic();
  }
  else
    exit();
  started = true;
}
