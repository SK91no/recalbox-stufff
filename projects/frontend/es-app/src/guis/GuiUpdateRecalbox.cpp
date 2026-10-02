//
// Created by xizor on 20/05/18.
//

#include <RecalboxConf.h>
#include "GuiUpdateRecalbox.h"
#include "components/TextComponent.h"
#include "components/MenuComponent.h"
#include <utils/Files.h>
#include <utils/locale/LocaleHelper.h>
#include "utils/network/HttpUnxzUntar.h"
#include <MainRunner.h>
#include <sys/stat.h>

#define BUTTON_GRID_VERT_PADDING Renderer::Instance().DisplayHeightAsFloat() * 0.025f
#define BUTTON_GRID_HORIZ_PADDING 10

#define TITLE_HEIGHT (mTitle->getFont()->Height() + TITLE_VERT_PADDING)

GuiUpdateRecalbox::GuiUpdateRecalbox(WindowManager& window, const String& tarUrl, const String& imageUrl, const String& sha1Url, const String& newVersion)
  : Gui(window)
  , mTarRequest(Path(sDownloadFolder))
  , mTarUrl(tarUrl)
  , mImageUrl(imageUrl)
  , mSha1Url(sha1Url)
  , mNewVersion(newVersion)
  , mTotalSize(0)
  , mCurrentSize(0)
  , mSender(*this)
  , mGrid(window, Vector2i(3, 4))
{
  mRebootIn = _("REBOOT IN %s");
  mError = _("Error downloading Recalbox %s... Please retry later!").Replace("%s", mNewVersion);

  addChild(&mGrid);

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  const float height = Renderer::Instance().DisplayHeightAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.7f : 0.5f);
  const float width = Renderer::Instance().DisplayWidthAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.8f : 0.6f);

  // Title
  mTitle = std::make_shared<TextScrollComponent>(mWindow, _("DOWNLOADING UPDATE..."), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true, Vector2i(1,1) );

  // Text
  String text = _("We're downloading __Recalbox__ version **%s**!\n\nOnce the download is complete, Recalbox will reboot and start installing the new version.\nTypical installations take about 5-10mn. **DO NOT reboot or power off Recalbox** until the installation is complete.")
                     .Replace("%s", newVersion);
  mText = std::make_shared<MarkdownText>(mWindow, text, *menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::CenterLeft);
  mGrid.setEntry(mText, Vector2i(1, 1), false, true, Vector2i(1,1) );

  // Progress bar
  mBar = std::make_shared<ProgressBarComponent>(mWindow, 1);
  mGrid.setEntry(mBar, Vector2i(1, 2), false, true, Vector2i(1,1) );

  // ETA
  mEta = std::make_shared<TextScrollComponent>(mWindow, _("START DOWNLOADING..."), menuTheme.SmallText().font, menuTheme.SmallText().color, ::Alignment::CenterLeft);
  mGrid.setEntry(mEta, Vector2i(1, 3), false, true, Vector2i(1,1) );

  mGrid.setColWidthPerc(0, 0.02f);
  mGrid.setColWidthPerc(1, 0.96f);
  mGrid.setColWidthPerc(2, 0.02f);

  // Set grid size
  float titlePercent = ((float)mTitle->getFont()->Height() * 1.6f) / height;
  float pbPercent = 0.08f;
  float etaPercent = ((float)mEta->getFont()->Height() * 2.0f) / height;
  float textPercent = 1.0f - (titlePercent + pbPercent + etaPercent);
  constexpr float mainColWidth = 0.96f;
  constexpr float marginWidth = (1.0f - mainColWidth) / 2;

  mGrid.setColWidthPerc(0, marginWidth, false);
  mGrid.setColWidthPerc(1, mainColWidth, false);
  mGrid.setColWidthPerc(2, marginWidth, false);
  mGrid.setRowHeightPerc(0, titlePercent, false);
  mGrid.setRowHeightPerc(1, textPercent, false);
  mGrid.setRowHeightPerc(2, pbPercent, false);
  mGrid.setRowHeightPerc(3, etaPercent, false);

  mText->setSize(width * mainColWidth, 0);

  mGrid.setSize(width, height);

  // Window
  setSize(width, height);
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - width) / 2,
              (Renderer::Instance().DisplayHeightAsFloat() - height) / 2);

  // Avoid sleeping!
  mIsProcessing = true;

  // start the thread if not aleady done
  Thread::Start("DownloadUpdate");
}

GuiUpdateRecalbox::~GuiUpdateRecalbox()
{
  Thread::Stop();
}

bool GuiUpdateRecalbox::ProcessInput(const InputCompactEvent& event)
{
  if (event.CancelReleased())
  {
    mTarRequest.Cancel();
    mImgRequest.Cancel();
    Close();
  }
  return Component::ProcessInput(event);
}

bool GuiUpdateRecalbox::CollectHelpItems(Help& help)
{
  mGrid.CollectHelpItems(help);
  help.Set(Help::Cancel(), _("CANCEL"));
  return true;
}

void GuiUpdateRecalbox::Run()
{
  { LOG(LogDebug) << "[UpdateGui] Download update file"; }

  // Empty target folder
  RecalboxSystem::EmptyUpdateFolder();

  // Download size
  HttpClient sizeRequest;
  Path sizeUrl = Path(mTarUrl).Directory() / "recalbox.boot.size";
  String sizeString;
  sizeRequest.Execute(sizeUrl.ToString(), sizeString);
  long long int size = sizeString.Trim("\r\n").AsInt64();

  // Get boot partition free size
  std::pair<String, int> result = RecalboxSystem::execute("df -P /boot");
  bool check = size > (1 << 30);
  bool spaceOk = false;
  if (result.second == 0)
    if (std::vector lines = result.first.Split('\n'); lines.size() > 1)
      if (std::vector chunks = lines[1].Split(' ', true); chunks.size() >= 4)
      {
        long long int freeSize = chunks[3].AsInt64() << 10;
        check = freeSize > 0;
        spaceOk = freeSize > size + (512 << 10 );
      }
  if (!check) mSender.Send(1);
  else if (!spaceOk) { mSender.Send(2); return; }

  // Ensure boot partition is R/W
  if (!RecalboxSystem::MakeBootReadWrite())
  {
    { LOG(LogError) << "[UpdateGui] Cannot mount /boot RW!"; }
    mSender.Send(-1);
    return;
  }

  // Get arch
  String arch = Files::LoadFile(Path("/recalbox/recalbox.arch"));
  if (arch == "xu4") arch = "odroidxu4";

  mTimeReference = DateTime();
  // First try stream update
  if (mTarRequest.SimpleExecute(mTarUrl, this))
  {
    struct stat sb;
    // execute pre-upgrade.sh
    if (stat(PRE_UPGRADE_SCRIPT, &sb) == 0) {
      String cmd = "bash " + String(PRE_UPGRADE_SCRIPT);
      { LOG(LogInfo) << "[Update] Executing " << cmd << " script"; }
      if (system(cmd.c_str()) != 0)
      { LOG(LogInfo) << "[Update] Error executing " << cmd << " script"; }
    }
    // Reboot
    MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot);
    return;
  }

  // If stream update fails with any other error than 404,
  // stop here. Let user start stream update again
  if (mTarRequest.GetLastHttpResponseCode() != 404)
  {
    { LOG(LogError) << "[UpdateGui] Stream update failed with " << mTarRequest.GetLastHttpResponseCode(); }
    mSender.Send(-1);
    return;
  }

  // If stream update fails with a 404, then fallback to legacy
  // image download.
  // Check free bytes on share partition
  if (RecalboxSystem::isFreeSpaceLimit())
  {
    String message = _("You must have at least %dGB free on 'SHARE' partition!").Replace("%d", String(RecalboxSystem::GetMinimumFreeSpaceOnSharePartition() >> 30));
    mWindow.displayMessage(message);
    Close();
    mSender.Send(-1);
    return;
  }

  // Get destination filename
  String destinationFileName = "recalbox-%.img.xz";
  destinationFileName.Replace("%", arch);

  // Download
  Path destination = Path(sDownloadFolder) / destinationFileName;
  Path destinationSha1 = Path(sDownloadFolder) / destinationFileName.Append(".sha1");
  { LOG(LogDebug) << "[UpdateGui] Target path " << destination.ToString(); }

  // Empty target folder
  RecalboxSystem::EmptyUpdateFolder();

  // Ensure boot partition is R/W
  if (!RecalboxSystem::MakeBootReadWrite())
  {
    { LOG(LogError) << "[UpdateGui] Cannot mount /boot RW!"; }
    mSender.Send(-1);
    return;
  }

  mTimeReference = DateTime();
  mImgRequest.Execute(mImageUrl, destination, this);

  // Control & Reboot
  if (mTotalSize != 0)
    if (destination.Size() == mTotalSize)
    {
      // Download sha1
      mImgRequest.Execute(mSha1Url.Append(".sha1"), destinationSha1, this);
      // Reboot
      MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot);
      return;
    }

  (void)destination.Delete();
  mSender.Send(-1);
}

void GuiUpdateRecalbox::DownloadProgress(const HttpClient&, long long int currentSize, long long int expectedSize)
{
  // Store data and synchronize
  mTotalSize = expectedSize;
  mCurrentSize = currentSize;
  mSender.Send(0);
}

void GuiUpdateRecalbox::ReceiveSyncMessage(int code)
{
  if (code == 2)
  {
    mWindow.displayScrollMessage(_("Unable to update"), (_F(_("Recalbox adds many new features with each new version, which gradually increases the size of the system.\nThe system partition on your installation is too small to accommodate version {0}.\n\nTo continue receiving updates, you must reinstall Recalbox by downloading the latest image from www.recalbox.com.")) / mNewVersion).ToString());
    Close();
  }
  if (code == 1)
  {
    mWindow.InfoPopupAddRegular(_("Unable to verify that there is enough space left on the boot partition. The update may not complete successfully."), 15, PopupType::Warning, false);
  }
  else if (code == 0)
  {
    // Load size into progress bar component
    mBar->setMaxValue(mTotalSize);
    mBar->setCurrentValue(mCurrentSize);

    // Elapsed time
    if (mCurrentSize != 0)
    {
      TimeSpan elapsed = DateTime() - mTimeReference;
      TimeSpan eta((elapsed.TotalMilliseconds() * (mTotalSize - mCurrentSize)) / mCurrentSize);

      String text = mRebootIn;
      text.Replace("%s", eta.ToTimeString());
      mEta->setText(text);
    }
  }
  else if (code < 0)
  {
    mEta->setText(mError);
    mGrid.onSizeChanged();
    // Empty target folder
    RecalboxSystem::EmptyUpdateFolder();
  }
}

