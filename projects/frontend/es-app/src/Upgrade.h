#pragma once

#include <utils/os/system/Thread.h>
#include <utils/sync/SyncMessageSender.h>
#include <utils/os/system/Signal.h>
#include <utils/Files.h>
#include <guis/GuiMsgBoxScroll.h>
#include <guis/GuiUpdateRecalbox.h>
#include <WindowManager.h>
#include "utils/locale/LocaleHelper.h"
#include "IManualUpdateCheckInterface.h"

// Forward declaration
class WindowManager;

class Upgrade: public StaticLifeCycleControler<Upgrade>
             , private Thread
             , private ISyncMessageReceiver<bool>
{
  public:
    //! Local version file
    static constexpr const char* sLocalReleaseNoteFile = "/recalbox/recalbox.releasenotes";
    //! Local version file
    static constexpr const char* sLocalVersionFile = "/recalbox/recalbox.version";
    //! Local version arch
    static constexpr const char* sLocalArchFile = "/recalbox/recalbox.arch";
    //! Local version arch
    static constexpr const char* sLocalUUID = "/recalbox/share/system/uuid";

    /*!
     * @brief Constructor
     * @param window main Window
     * @param firstRun True if the front end just lauched, false otherwise
     */
    Upgrade(WindowManager& window, bool firstRun);

    /*!
     * @brief Destructor
     */
    ~Upgrade() override;

    /*!
     * @brief Return remote version.
     * @return Remote version
     */
    String NewVersion() { return mRemoteVersion.empty() ? mLocalVersion : mRemoteVersion; }

    /*!
     * @brief Return trimmed current version, read at runtime from sLocalVersionFile, falls back to the built-in version
     * @return Current version
     */
    static String CurrentVersion();

    /*!
     * @brief Return trimmed current version.
     * @return Current version
     */
    static String CurrentArch() { return Files::LoadFile(Path(sLocalArchFile)).Trim(" \t\r\n").Replace('_', '/'); }

    /*!
     * @brief Return remote releasenote
     * @return Remote release note
     */
    String NewReleaseNote() { return mRemoteReleaseNote.empty() ? mLocalReleaseNote : mRemoteReleaseNote; }

    /*!
     * @brief Return Tar url
     * @return tar url
     */
    String TarUrl();

    /*!
     * @brief Return remote version.
     * @return Remote version
     */
    String ImageUrl();

    /*!
     * @brief Return remote version.
     * @return Remote version
     */
    String HashUrl();

    /*!
     * @brief Check if there is a pending update
     */
    bool PendingUpdate()
    {
      if (mRemoteVersion.empty()) return false;
      return mRemoteVersion != mLocalVersion;
    }

    /*!
     * @brief Check if the network is ready
     * @return True if the network is ready
     */
    bool NetworkReady() { return !GetDomainName().empty(); }

    /*!
     * @brief Check if an update is available, immediately
     */
    void DoManualCheck(IManualUpdateCheckInterface* callback);

  private:
    class UpdatePopup : public GuiMsgBoxScroll
    {
      public:
        //! Build & show the popup if it does not already exists
        static void Show(WindowManager* window, Upgrade* upgrade, const String& message)
        {
          if (mInstance == nullptr && !window->DoNotDisturb())
          {
            mInstance = new UpdatePopup(window, upgrade, message);
            window->pushGui(mInstance);
          }
        }

        /// Destructor - remove unique instance
        ~UpdatePopup() { mInstance = nullptr; }

      private:
        //! Static unitary instance
        static UpdatePopup* mInstance;

        //! Launch update window
        static void LaunchUpdate(WindowManager* window, Upgrade* upgrade)
        {
          window->pushGui(new GuiUpdateRecalbox(*window, upgrade->TarUrl(), upgrade->ImageUrl(), upgrade->HashUrl(), upgrade->NewVersion()));
        }

        //! Default constructor
        UpdatePopup(WindowManager* window, Upgrade* upgrade, const String& message)
          : GuiMsgBoxScroll(*window, _("AN UPDATE IS AVAILABLE FOR YOUR RECALBOX"), message, _("LATER"), nullptr, _("UPDATE NOW"),
                            std::bind(UpdatePopup::LaunchUpdate, window, upgrade), String::Empty, nullptr, Alignment::CenterLeft) {}
    };

    //! Release DNS
    static constexpr const char* sUpgradeDNS = ".download.recalbox.com";

    //! Get remote version template URL
    static constexpr const char* sVersionPatternUrl = "https://#DOMAIN#/latest/#ARCH#/recalbox.version?source=recalbox&uuid=#UUID#";
    //! Data template url
    static constexpr const char* sDownloadPatternUrl = "https://#DOMAIN#/latest/#ARCH#/recalbox-#ARCH#.img.xz#EXT#?source=recalbox&uuid=#UUID#";
    static constexpr const char* sTarDownloadPatternUrl = "https://#DOMAIN#/latest/#ARCH#/recalbox-#ARCH#.tar.xz#EXT#?source=recalbox&uuid=#UUID#";
    //! Releasenote template url
    static constexpr const char* sReleasenotePatternUrl = "https://#DOMAIN#/latest/#ARCH#/recalbox.releasenotes";

    //! MainWindow
    WindowManager& mWindow;
    //! Syncronous event to display popup
    SyncMessageSender<bool> mSender;
    //! Signal used to stop the thread
    Signal mSignal;
    //! Built popup message
    String mPopupMessage;
    //! Build MessageBox message
    String mMessageBoxMessage;
    //! Download URL
    String mDomainName;
    //! Remote version
    String mRemoteVersion;
    //! Local version
    String mLocalVersion;
    //! Remote version
    String mRemoteReleaseNote;
    //! Local version
    String mLocalReleaseNote;
    //! First run?
    bool mFirstRun;

    //! Manual check required
    IManualUpdateCheckInterface* mManualCheckInterface;

    /*
     * Thread implementation
     */

    /*!
     * Thread run method
     */
    void Run() override;

    /*
     * Synchronous event implementation
     */

    /*!
     * @brief Receive synchronous message
     */
    void ReceiveSyncMessage(bool value) override;

    /*!
     * @brief Get update url from DNS TXT records
     */
    String GetDomainName();

    /*!
     * @brief Get remote version
     */
    String GetRemoteVersion();

    /*!
     * @brief Get remote version
     */
    String GetRemoteReleaseVersion();

    /*!
     * @brief Replace machine parameters parameters in the given url (Arch, uuid, domain, ...)
     * @param url Template url
     * @param ext Optional extention
     * @return Final url
     */
    String ReplaceMachineParameters(const String& url, const String& ext);

    /*!
     * @brief Validate the given version
     * @param version Version to validate
     * @return True if the given version has been identified as valid, false otherwise
     */
    static bool ValidateVersion(const String& version);
};


