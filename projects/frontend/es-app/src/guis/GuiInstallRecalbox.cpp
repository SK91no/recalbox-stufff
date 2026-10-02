//
// Created by bkg2k on 24/04/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "GuiInstallRecalbox.h"
#include <utils/locale/LocaleHelper.h>
#include <utils/Files.h>
#include <utils/Sizes.h>
#include <MainRunner.h>
#include <cstdio>
#include <algorithm>
#include <systems/SystemManager.h>

// Per-source skip lists: directories whose container is recreated on the target,
// but whose contents are deliberately NOT copied (rebuilt at runtime, volatile,
// or simply not desired on a fresh install).
static const std::set<String> sBootSkipContents;
static const std::set<String> sOverlaySkipContents = {
    "/overlay/lower",
    "/overlay/work",
};
static const std::set<String> sShareSkipContents = {
    "/recalbox/share/arcade",
    "/recalbox/share/bootvideos",
    "/recalbox/share/extractions",
};

GuiInstallRecalbox::GuiInstallRecalbox(WindowManager& window, SystemManager& systemManager, const String& devicePath, const String& deviceName)
  : Gui(window)
  , mSystemManager(systemManager)
  , mDevicePath(devicePath)
  , mDeviceName(deviceName)
  , mInstallComplete(false)
  , mProgress(0)
  , mFilesCopied(0)
  , mBytesCopied(0LL)
  , mTotalCopyBytes(0LL)
  , mSender(*this)
  , mBackground(window, Path(":/frame.png"))
  , mGrid(window, Vector2i(3, 4))
{
  mErrorText = _("Installation failed! Check the logs for details.");

  addChild(&mBackground);
  addChild(&mGrid);

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  const float height = Renderer::Instance().DisplayHeightAsFloat() * (Renderer::Instance().Is480pOrLower() ? 0.7f : 0.5f);
  const float width  = Renderer::Instance().DisplayWidthAsFloat()  * (Renderer::Instance().Is480pOrLower() ? 0.8f : 0.6f);

  // Title
  mTitle = std::make_shared<TextScrollComponent>(mWindow, _("INSTALLATION IN PROGRESS..."),
                                                 menuTheme.Title().font, menuTheme.Title().color,
                                                 ::Alignment::Center);
  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true, Vector2i(1, 1));

  // Body text (markdown)
  String text = _("Installing Recalbox on **{DEVICE}**...\n\n"
                  "⚠ **Do NOT** shut down or reboot Recalbox during the installation process.\n"
                  "If the installation is interrupted, the disk may be left in an unusable state.\n\n"
                  "Please wait — this may take **several minutes**.");
  text.Replace("{DEVICE}", deviceName);
  mText = std::make_shared<MarkdownText>(mWindow, text,
                                         *menuTheme.SmallText().font, menuTheme.SmallText().color,
                                         ::Alignment::CenterLeft);
  mGrid.setEntry(mText, Vector2i(1, 1), false, true, Vector2i(1, 1));

  // Progress bar
  mBar = std::make_shared<ProgressBarComponent>(mWindow, 100);
  mGrid.setEntry(mBar, Vector2i(1, 2), false, true, Vector2i(1, 1));

  // Status line
  mStatus = std::make_shared<TextScrollComponent>(mWindow, _("Starting installation..."),
                                                   menuTheme.SmallText().font, menuTheme.SmallText().color,
                                                   ::Alignment::CenterLeft);
  mGrid.setEntry(mStatus, Vector2i(1, 3), false, true, Vector2i(1, 1));

  // Background frame
  mBackground.setImagePath(menuTheme.Background().path);
  mBackground.setCenterColor(menuTheme.Background().color);
  mBackground.setEdgeColor(menuTheme.Background().color);
  mBackground.fitTo({ width, height }, Vector3f::Zero(), Vector2f(-32, -32));

  // Column layout: thin margin | main content | thin margin
  constexpr float mainColWidth = 0.96f;
  constexpr float marginWidth  = (1.0f - mainColWidth) / 2.0f;
  mGrid.setColWidthPerc(0, marginWidth,  false);
  mGrid.setColWidthPerc(1, mainColWidth, false);
  mGrid.setColWidthPerc(2, marginWidth,  false);

  // Row layout: title | text | progress bar | status
  const float titlePercent  = ((float)mTitle->getFont()->Height()  * 1.6f) / height;
  const float statusPercent = ((float)mStatus->getFont()->Height() * 2.0f) / height;
  constexpr float pbPercent = 0.08f;
  const float textPercent   = 1.0f - (titlePercent + pbPercent + statusPercent);
  mGrid.setRowHeightPerc(0, titlePercent,  false);
  mGrid.setRowHeightPerc(1, textPercent,   false);
  mGrid.setRowHeightPerc(2, pbPercent,     false);
  mGrid.setRowHeightPerc(3, statusPercent, false);

  mText->setSize(width * mainColWidth, 0);

  mGrid.setSize(width, height);
  setSize(width, height);
  setPosition((Renderer::Instance().DisplayWidthAsFloat()  - width)  / 2.0f,
              (Renderer::Instance().DisplayHeightAsFloat() - height) / 2.0f);

  mIsProcessing = true;

  Thread::Start("InstallRecalbox");
}

GuiInstallRecalbox::~GuiInstallRecalbox()
{
  Thread::Stop();
}

bool GuiInstallRecalbox::ProcessInput(const InputCompactEvent& event)
{
  if (mInstallComplete)
  {
    if (event.AnyButtonReleased())
      MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot);
  }
  return Component::ProcessInput(event);
}

bool GuiInstallRecalbox::CollectHelpItems(Help& help)
{
  mGrid.CollectHelpItems(help);
  if (mInstallComplete)
    help.Set(Help::Valid(), _("REBOOT"));
  return true;
}

void GuiInstallRecalbox::Update(int deltaTime)
{
  Component::Update(deltaTime);

  // Drive the status line and progress bar from the atomic counters while a copy phase is active.
  // mTotalCopyBytes == 0 means the install thread is in a non-copy phase: SyncMessage owns the UI.
  long long total = mTotalCopyBytes.load(std::memory_order_acquire);
  if (total <= 0LL) return;

  long long bytes = mBytesCopied.load(std::memory_order_relaxed);
  int       files = mFilesCopied.load(std::memory_order_relaxed);

  String text = _("{COUNT} files copied ({DONE}/{TOTAL})");
  text.Replace("{COUNT}", String(files));
  text.Replace("{DONE}",  Sizes(bytes).ToHumanSize());
  text.Replace("{TOTAL}", Sizes(total).ToHumanSize());
  mStatus->setText(text);

  long long range = (long long)(sCopyEndPct - sCopyStartPct);
  long long pct   = (long long)sCopyStartPct + (bytes * range) / total;
  if (pct > sCopyEndPct) pct = sCopyEndPct;
  mBar->setMaxValue(100);
  mBar->setCurrentValue(pct);
}

void GuiInstallRecalbox::SetProgress(int percent, const String& status)
{
  {
    Mutex::AutoLock lock(mStatusMutex);
    mProgress      = percent;
    mCurrentStatus = status;
  }
  mSender.Send(sStatusProgress);
}

bool GuiInstallRecalbox::RunCommand(const String& cmd)
{
  { LOG(LogInfo) << "[InstallRecalbox] " << cmd; }
  #ifdef DEBUG
  Thread::Sleep(1000);
  return true;
  #else
  return system(cmd.c_str()) == 0;
  #endif
}

String GuiInstallRecalbox::RunCapture(const String& cmd)
{
  FILE* f = popen(cmd.c_str(), "r");
  if (f == nullptr) return String();
  char buf[256];
  String result;
  while (fgets(buf, (int)sizeof(buf), f) != nullptr)
    result.Append(buf);
  pclose(f);
  return result.Trim();
}

long long GuiInstallRecalbox::GetDirSizeBytes(const Path& dir)
{
  // "du -sbx" prints "<bytes>\t<path>" — grab just the byte count.
  // -x stays on the same filesystem so the measurement matches CopyTree's mount-point skip.
  String out = RunCapture(String("du -sbx ").Append(dir.ToString()).Append(" 2>/dev/null"));
  String::List parts = out.Split('\t');
  if (parts.empty()) return 0LL;
  return parts[0].AsInt64();
}

String GuiInstallRecalbox::PartitionPath(int n) const
{
  // NVMe and MMC device names end in a digit and need a 'p' before the partition number
  String devName = Path(mDevicePath).Filename();
  String path(mDevicePath);
  if (devName.StartsWith("nvme") || devName.StartsWith("mmcblk"))
    path.Append('p');
  return path.Append(String(n));
}

void GuiInstallRecalbox::Unmount(const Path& mountRecalbox, const Path& mountShare, const Path& mountOverlay)
{
  RunCommand(String("umount ").Append(mountRecalbox.ToString()));
  RunCommand(String("umount ").Append(mountShare.ToString()));
  RunCommand(String("umount ").Append(mountOverlay.ToString()));
}

bool GuiInstallRecalbox::IsTargetDeviceNode(const String& src) const
{
  // True if 'src' is the target device itself or one of its partitions
  // (handles /dev/sda + /dev/sdaN, and /dev/nvme0n1 / /dev/mmcblk0 + their /…pN partitions).
  if (src == mDevicePath) return true;
  if (!src.StartsWith(mDevicePath)) return false;
  int i = mDevicePath.Count();
  if (i >= src.Count()) return false;
  if (src[i] == 'p' && i + 1 < src.Count()) ++i; // nvme/mmcblk style
  return src[i] >= '0' && src[i] <= '9';
}

void GuiInstallRecalbox::UnmountTargetDevice()
{
  // Disable any swap whose backing device is the target or one of its partitions.
  // /proc/swaps format: "Filename Type Size Used Priority" — kernel uses tabs as separators
  // and the first line is a header.
  String::List swaps = Files::LoadAllFileLines(Path("/proc/swaps"));
  for (size_t i = 1; i < swaps.size(); ++i)
  {
    String line = swaps[i]; line.Replace('\t', ' ');
    String::List fields = line.Split(' ', true);
    if (fields.empty()) continue;
    if (IsTargetDeviceNode(fields[0]))
      RunCommand(String("swapoff ").Append(fields[0]));
  }

  // Collect every mount point whose source device matches the target.
  // /proc/mounts format: "<source> <mountpoint> <fstype> <opts> <dump> <pass>".
  String::List mountPoints;
  for (const String& line : Files::LoadAllFileLines(Path("/proc/mounts")))
  {
    String::List fields = line.Split(' ', true);
    if (fields.size() < 2) continue;
    if (IsTargetDeviceNode(fields[0]))
      mountPoints.push_back(fields[1]);
  }

  // Unmount deepest paths first so nested mounts come off before their parents.
  std::sort(mountPoints.begin(), mountPoints.end(), [](const String& a, const String& b) { return a > b; });

  // Best-effort: try a clean umount, fall back to lazy umount if the target is busy.
  for (const String& mp : mountPoints)
    if (!RunCommand(String("umount ").Append(mp)))
      RunCommand(String("umount -l ").Append(mp));
}

void GuiInstallRecalbox::LoadMountPoints()
{
  mMountPoints.clear();
  for (const String& line : Files::LoadAllFileLines(Path("/proc/mounts")))
  {
    String::List fields = line.Split(' ', true);
    if (fields.size() < 2) continue;
    mMountPoints.insert(fields[1]);
  }
}

bool GuiInstallRecalbox::CopyTree(const Path& src, const Path& dst, const std::set<String>& skipContents, bool dryRun)
{
  { LOG(LogDebug) << "[GuiInstallRecalbox] Copying " << src << " to " << dst; }

  // In dry-run we only walk the tree to keep the byte counter progressing — no I/O on the target.
  if (!dryRun)
  {
    if (!dst.CreatePath())
    {
      { LOG(LogError) << "[GuiInstallRecalbox] Error creating " << dst; }
      return false;
    }
  }

  // Directory explicitly listed: dst was just created above, but switch to dry-run for the
  // whole subtree so files are counted (and the bar keeps moving) without being written out.
  if (!dryRun && skipContents.find(src.ToString()) != skipContents.end())
  {
    { LOG(LogInfo) << "[GuiInstallRecalbox] Repertoire non copié explicitement: " << src; }
    dryRun = true;
  }

  for (const Path& entry : src.GetDirectoryContent())
  {
    // Never cross filesystem boundaries: skip any entry that is itself a mount point
    if (mMountPoints.find(entry.ToString()) != mMountPoints.end())
    {
      { LOG(LogInfo) << "[GuiInstallRecalbox] Skipping mount point: " << entry; }
      continue;
    }
    if (entry.IsFile())
    {
      long long size = entry.Size();
      if (!dryRun)
      {
        #ifdef DEBUG
        Thread::Sleep(1);
        #else
        if (!Files::CopyFile(entry, dst / entry.Filename()))
        {
          { LOG(LogError) << "[GuiInstallRecalbox] Error copying " << entry << " to " << (dst / entry.Filename()); }
          return false;
        }
        #endif
      }
      mBytesCopied.fetch_add(size, std::memory_order_relaxed);
      mFilesCopied.fetch_add(1,    std::memory_order_relaxed);
    }
    else if (entry.IsDirectory())
    {
      if (!CopyTree(entry, dst / entry.Filename(), skipContents, dryRun))
        return false;
    }
  }
  return true;
}

void GuiInstallRecalbox::Run()
{
  // Disable mount watcher
  mSystemManager.GetMountMonitor().SetMonitoringState(false);

  // Read disk geometry 
  String devName = Path(mDevicePath).Filename();
  long long totalSectors = Files::LoadFile(Path(String("/sys/block/").Append(devName).Append("/size"))).Trim().AsInt64();
  long long totalMiB = (totalSectors * 512LL) / (1024LL * 1024LL);
  if (totalMiB < 7200LL)
  {
    { LOG(LogError) << "[InstallRecalbox] Disk too small or unreadable: " << totalMiB << " MiB"; }
    mSender.Send(sStatusError);
    return;
  }

  // OVERLAY occupies the last 1025 MiB; SHARE fills the gap between RECALBOX and OVERLAY
  long long overlaySectorMiB = totalMiB - 1025LL;

  // Pre-check: /recalbox/share must fit in the SHARE partition 
  SetProgress(0, _("Checking available disk space..."));
  long long shareContentBytes  = GetDirSizeBytes(Path("/recalbox/share"));
  long long sharePartitionBytes = (overlaySectorMiB - 6145LL) * 1024LL * 1024LL;
  // Reserve 3% for exFAT metadata overhead
  if (shareContentBytes > sharePartitionBytes * 97LL / 100LL)
  {
    String msg = _("Not enough space on the target disk.\n\n"
                   "The content of **/recalbox/share** ({SIZE_SHARE}) does not fit in the available "
                   "SHARE partition ({SIZE_AVAIL}).\n\n"
                   "Please use a larger disk or free up space on /recalbox/share first.");
    msg.Replace("{SIZE_SHARE}", Sizes(shareContentBytes).ToHumanSize());
    msg.Replace("{SIZE_AVAIL}", Sizes(sharePartitionBytes * 97LL / 100LL).ToHumanSize());
    {
      Mutex::AutoLock lock(mStatusMutex);
      mCurrentStatus = msg;
    }
    mSender.Send(sStatusSizeError);
    return;
  }

  // Step 1a (0%): unmount any filesystem currently mounted from the target device 
  // parted refuses to relabel a disk that still has mounted partitions, so detach everything
  // first. This is best-effort: swapoff and umount errors are tolerated and the next step will
  // surface a real failure if the device is genuinely unusable.
  SetProgress(0, _("Unmounting existing partitions..."));
  UnmountTargetDevice();
  RunCommand("udevadm settle");

  // Stop automount so that we are sure it will not remount new partitions
  Files::SaveFile(Path("/tmp/disable.automount"), String('1'));

  // Step 1b (0%): wipe existing partition table 
  SetProgress(0, _("Wiping existing partitions..."));
  if (!RunCommand(String("parted -s ").Append(mDevicePath).Append(" mklabel msdos")))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error initializing MBR disk."; } mSender.Send(sStatusError); return; }

  // Step 2 (3%): create RECALBOX partition — FAT32, 6 GB, start of disk
  SetProgress(3, _("Creating RECALBOX partition (FAT32, 6 GB)..."));
  if (!RunCommand(String("parted -s ").Append(mDevicePath).Append(" mkpart primary fat32 1MiB 6145MiB")))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error creating boot partition."; } mSender.Send(sStatusError); return; }
  if (!RunCommand(String("parted -s ").Append(mDevicePath).Append(" set 1 boot on")))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error setting boot flag on RECALBOX partition."; } mSender.Send(sStatusError); return; }

  // Step 3 (6%): create SHARE partition — exFAT, all space in between 
  SetProgress(6, _("Creating SHARE partition (exFAT)..."));
  if (!RunCommand(String("parted -s ").Append(mDevicePath)
                    .Append(" mkpart primary ntfs 6145MiB ").Append(String(overlaySectorMiB)).Append("MiB")))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error creating share partition."; } mSender.Send(sStatusError); return; }

  // Step 4 (9%): create OVERLAY partition — ext4, 1 GB, end of disk 
  SetProgress(9, _("Creating OVERLAY partition (ext4, 1 GB)..."));
  if (!RunCommand(String("parted -s ").Append(mDevicePath)
                    .Append(" mkpart primary ext4 ").Append(String(overlaySectorMiB)).Append("MiB 100%")))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error creating overlay partition."; } mSender.Send(sStatusError); return; }

  // Step 5 (12%): reload partition table so kernel sees new nodes 
  SetProgress(12, _("Reloading partition table..."));
  RunCommand(String("partprobe ").Append(mDevicePath));
  RunCommand("udevadm settle");

  // Step 6 (15%): format RECALBOX as FAT32 
  SetProgress(15, _("Formatting RECALBOX partition (FAT32)..."));
  if (!RunCommand(String("mkfs.fat -F 32 -n RECALBOX ").Append(PartitionPath(1))))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error formating boot partition."; } mSender.Send(sStatusError); return; }

  // Step 7 (18%): format SHARE as exFAT 
  SetProgress(18, _("Formatting SHARE partition (exFAT)..."));
  if (!RunCommand(String("mkfs.exfat -n SHARE ").Append(PartitionPath(2))))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error formating share partition."; } mSender.Send(sStatusError); return; }

  // Step 8 (21%): format OVERLAY as ext4 
  SetProgress(21, _("Formatting OVERLAY partition (ext4)..."));
  if (!RunCommand(String("mkfs.ext4 -F -L OVERLAY ").Append(PartitionPath(3))))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error formating overlay partition."; } mSender.Send(sStatusError); return; }

  // Step 9 (24%): create mount points and mount all three partitions 
  const Path mountBase("/tmp/recalbox-install");
  const Path mountRecalbox = mountBase / "recalbox";
  const Path mountShare    = mountBase / "share";
  const Path mountOverlay  = mountBase / "overlay";

  // Snapshot active mount points BEFORE mounting our targets so the target
  // paths themselves are not in the set (otherwise CopyTree would skip them).
  LoadMountPoints();

  SetProgress(24, _("Mounting partitions..."));
  (void)mountRecalbox.CreatePath();
  (void)mountShare.CreatePath();
  (void)mountOverlay.CreatePath();

  if (!RunCommand(String("mount -t vfat ").Append(PartitionPath(1)).Append(" ").Append(mountRecalbox.ToString())))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error cloning boot files."; } mSender.Send(sStatusError); return; }
  if (!RunCommand(String("mount -t exfat ").Append(PartitionPath(2)).Append(" ").Append(mountShare.ToString())))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error cloning share files."; } RunCommand(String("umount ").Append(mountRecalbox.ToString())); mSender.Send(sStatusError); return; }
  if (!RunCommand(String("mount -t ext4 ").Append(PartitionPath(3)).Append(" ").Append(mountOverlay.ToString())))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error cloning overlay files."; } RunCommand(String("umount ").Append(mountRecalbox.ToString())); RunCommand(String("umount ").Append(mountShare.ToString())); mSender.Send(sStatusError); return; }

  // Pre-measure all sources to compute proportional progress
  SetProgress(27, _("Measuring data to copy..."));
  long long bootSize    = GetDirSizeBytes(Path("/boot"));
  long long overlaySize = GetDirSizeBytes(Path("/overlay"));
  long long totalCopyBytes = bootSize + overlaySize + shareContentBytes;
  if (totalCopyBytes < 1LL) totalCopyBytes = 1LL;

  // Prime atomic counters and activate Update() driven UI refresh for the copy phase.
  mFilesCopied.store(0,              std::memory_order_relaxed);
  mBytesCopied.store(0LL,            std::memory_order_relaxed);
  mTotalCopyBytes.store(totalCopyBytes, std::memory_order_release);

  // Step 10: copy /boot → RECALBOX partition
  if (!CopyTree(Path("/boot"), mountRecalbox, sBootSkipContents))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error copying boot partition."; } Unmount(mountRecalbox, mountShare, mountOverlay); mTotalCopyBytes.store(0LL); mSender.Send(sStatusError); return; }

  // Step 11: copy /overlay → OVERLAY partition
  if (!CopyTree(Path("/overlay"), mountOverlay, sOverlaySkipContents))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error copying share partition."; } Unmount(mountRecalbox, mountShare, mountOverlay); mTotalCopyBytes.store(0LL); mSender.Send(sStatusError); return; }

  // Step 12: copy /recalbox/share → SHARE partition
  if (!CopyTree(Path("/recalbox/share"), mountShare, sShareSkipContents))
  { { LOG(LogError) << "[GuiInstallRecalbox] Error copying overlay partition."; } Unmount(mountRecalbox, mountShare, mountOverlay); mTotalCopyBytes.store(0LL); mSender.Send(sStatusError); return; }

  // Copy phase done: hand the status line back to the SyncMessage path.
  mTotalCopyBytes.store(0LL);

  // Finalise: sync buffers and unmount
  { LOG(LogError) << "[GuiInstallRecalbox] Installation completed successfully."; }
  SetProgress(99, _("Finalizing installation..."));
  RunCommand("sync");
  Unmount(mountRecalbox, mountShare, mountOverlay);

  mSender.Send(sStatusSuccess);
}

void GuiInstallRecalbox::ReceiveSyncMessage(int code)
{
  if (code == sStatusProgress)
  {
    String status;
    int progress = 0;
    {
      Mutex::AutoLock lock(mStatusMutex);
      status   = mCurrentStatus;
      progress = mProgress;
    }
    mStatus->setText(status);
    mBar->setMaxValue(100);
    mBar->setCurrentValue((long long)progress);
    mGrid.onSizeChanged();
  }
  else if (code == sStatusSuccess)
  {
    mInstallComplete = true;
    mBar->setMaxValue(100);
    mBar->setCurrentValue(100LL);
    mStatus->setText(_("Installation complete! Press any button to reboot."));
    mGrid.onSizeChanged();
  }
  else if (code == sStatusSizeError)
  {
    String msg;
    { Mutex::AutoLock lock(mStatusMutex); msg = mCurrentStatus; }
    mWindow.displayMessage(msg);
    (void)Path("/tmp/disable.automount").Delete();
    mSystemManager.GetMountMonitor().SetMonitoringState(true);
    Close();
  }
  else // sStatusError
  {
    mStatus->setText(mErrorText);
    mGrid.onSizeChanged();
    (void)Path("/tmp/disable.automount").Delete();
    mSystemManager.GetMountMonitor().SetMonitoringState(true);
  }
}
