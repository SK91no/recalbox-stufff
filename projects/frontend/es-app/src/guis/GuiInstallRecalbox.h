//
// Created by bkg2k on 24/04/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include "guis/Gui.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "components/MarkdownText.h"
#include "components/TextScrollComponent.h"
#include "components/ProgressBarComponent.h"
#include <themes/MenuThemeData.h>
#include <utils/os/system/Thread.h>
#include <utils/os/system/Mutex.h>
#include <utils/sync/SyncMessageSender.h>
#include <set>
#include <atomic>

class SystemManager;

class GuiInstallRecalbox : public Gui
                           , private Thread
                           , private ISyncMessageReceiver<int>
{
  public:
    GuiInstallRecalbox(WindowManager& window, SystemManager& systemManager, const String& devicePath, const String& deviceName);

    ~GuiInstallRecalbox() override;

    bool ProcessInput(const InputCompactEvent& event) override;

    bool CollectHelpItems(Help& help) override;

    void Update(int deltaTime) override;

  private:
    static constexpr int sStatusProgress  =  0;
    static constexpr int sStatusSuccess   =  1;
    static constexpr int sStatusError     = -1;
    static constexpr int sStatusSizeError = -2; //!< Fatal: not enough space; close + popup

    //! Receive UI update messages from the install thread
    void ReceiveSyncMessage(int code) override;

    //! Main install thread routine
    void Run() override;

    //! Set progress percentage [0..100] and status text; safe to call from the install thread
    void SetProgress(int percent, const String& status);

    //! Run a shell command; returns true if exit code is 0
    bool RunCommand(const String& cmd);

    //! Return the device node for partition n (e.g. /dev/sda1 or /dev/nvme0n1p1)
    [[nodiscard]] String PartitionPath(int n) const;

    //! Run a shell command and return its stdout output
    [[nodiscard]] static String RunCapture(const String& cmd);

    //! Return the total disk usage of a directory in bytes (uses du -sb)
    [[nodiscard]] static long long GetDirSizeBytes(const Path& dir);

    //! Recursively copy src into dst. Increments mFilesCopied / mBytesCopied as it goes; the
    //! status line and progress bar are refreshed by Update() reading those atomics.
    //! Any directory whose absolute path is in @p skipContents has its destination created on
    //! the target but its contents are intentionally not copied — the subtree is still walked
    //! so the byte counter keeps progressing, which is what @p dryRun signals on recursion.
    bool CopyTree(const Path& src, const Path& dst, const std::set<String>& skipContents, bool dryRun = false);

    //! Unmount all three install mount points (best-effort, ignores errors)
    void Unmount(const Path& mountRecalbox, const Path& mountShare, const Path& mountOverlay);

    //! Unmount every filesystem currently mounted from the target device or any of its partitions.
    //! Best-effort: tries a clean umount first, then a lazy umount as a fallback.
    void UnmountTargetDevice();

    //! True if @p src is the target device itself or one of its partitions.
    [[nodiscard]] bool IsTargetDeviceNode(const String& src) const;

    //! Populate mMountPoints with every active mount point read from /proc/mounts.
    void LoadMountPoints();

    SystemManager& mSystemManager;

    //! Target device path (e.g. /dev/sda)
    String mDevicePath;
    //! Human-readable device name for display
    String mDeviceName;
    //! Cached generic error string
    String mErrorText;
    //! Set to true once the install thread reports success (used by ProcessInput)
    bool   mInstallComplete;

    //! Current progress percentage [0..100], shared between install thread and UI thread
    int    mProgress;
    //! Current status description, shared between install thread and UI thread
    String mCurrentStatus;
    //! Protects mProgress and mCurrentStatus
    Mutex  mStatusMutex;

    //! Active mount points snapshot, loaded before copying so CopyTree can skip them
    std::set<String> mMountPoints;

    //! Bar range allocated to the copy phase (used by Update() to derive percentage from bytes)
    static constexpr int sCopyStartPct = 30;
    static constexpr int sCopyEndPct   = 99;

    //! Number of files copied so far (written by install thread, read by UI thread)
    std::atomic<int>       mFilesCopied;
    //! Number of bytes copied so far
    std::atomic<long long> mBytesCopied;
    //! Total bytes to copy. >0 signals the copy phase is active; Update() drives the UI from
    //! the atomics only while this is non-zero, otherwise the standard SyncMessage path owns it.
    std::atomic<long long> mTotalCopyBytes;

    SyncMessageSender<int> mSender;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextScrollComponent>  mTitle;
    std::shared_ptr<MarkdownText>         mText;
    std::shared_ptr<ProgressBarComponent> mBar;
    std::shared_ptr<TextScrollComponent>  mStatus;
};
