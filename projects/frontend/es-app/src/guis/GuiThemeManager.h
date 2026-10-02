//
// Created by bkg2k on 30/09/25.
//
#pragma once

#include "guis/Gui.h"
#include "WindowManager.h"
#include "components/NinePatchComponent.h"
#include "components/PictureGalery2DComponent.h"
#include "components/PictureComponent.h"
#include "components/TextScrollComponent.h"
#include "GuiWaitLongExecution.h"
#include "themes/ThemeData.h"
#include "components/MarkdownText.h"
#include "utils/network/HttpClient.h"
#include "utils/markdown/MarkdownTools.h"

enum class InstallError
{
  Ok,
  DownloadFailed,
  WriteFailed,
  RenameError,
};

class GuiThemeManager final : public Gui
                            , public ILongExecution<bool, int> // Load theme descriptor bar
                            , public ILongExecution<int, InstallError> // Theme installation bar
                            , public PictureGalery2DComponent::IChangeNotifications
                            , public HttpClient::IDownload
                            , public IThemeSwitchable
                            , private IHttpConfiguration
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     * @param resolver Global variable resolver
     */
    explicit GuiThemeManager(WindowManager& window, IGlobalVariableResolver& resolver);

    ~GuiThemeManager() final;

  private:
    //! Installation temp folder
    static constexpr const char* sInstallationTmpFolder = "/boot/update";
    //! Installation file
    static constexpr const char* sInstalledFromFileName = ".installedFrom";
    //! Local theme path
    static constexpr const char* sLocalThemeFolder = "/recalbox/share/themes";
    //! Theme hub domain
    static constexpr const char* sThemeHubDomain = "theme-hub.recalbox.com";
    //! Patron Theme hub domain
    static constexpr const char* sPatronThemeHubDomain = "theme-hub-patron.recalbox.com";

    enum class ThemeType
    {
      Embedded,          //!< Embedded theme
      Remote,            //!< Theme in remote repository
      ManuallyInstalled, //!< Manually installed theme
    };

    /*
     * Installed theme holder
     */
    class InstalledThemes
    {
      public:
        InstalledThemes(const Path& installedFrom, const Path& installedIn, int version, bool compatible)
         : mInstalledFrom(installedFrom)
         , mInstalledIn(installedIn)
         , mInstalledVersion(version)
         , mCompatible(compatible)
        {}

        //! Get url from whuch the theme was installed
        [[nodiscard]] const Path& InstalledFrom() const { return mInstalledFrom; }
        //! Get folder in which the theme is installed
        [[nodiscard]] const Path& InstalledIn() const { return mInstalledIn; }
        //! Get installed version
        [[nodiscard]] int Version() const { return mInstalledVersion; }
        //! Get compatibility flag
        [[nodiscard]] bool IsCompatible() const { return mCompatible; }

      private:
        Path mInstalledFrom;  //!< Installated from URL
        Path mInstalledIn;    //!< Installed in this path
        int mInstalledVersion; //!< Installed version
        bool mCompatible;     //!< This theme is compatible ?
    };

    /*
     * Local theme information
     */
    struct LocalTheme
    {
      String mDescriptorFolder;   //!< Descriptor folder (& screenshots)
      InstalledThemes mInstalled; //! Fake installation structure
    };

    /*!
     * @brief Raw item from the global theme lists
     */
    class ThemeListItem
    {
      public:
        /*!
         * @brief Constructor
         * @param folder Folder
         * @param active Active ?
         */
        ThemeListItem(const String& baseUrl, const String& folder, bool active, ThemeType type, bool patron) : mBaseUrl(baseUrl), mFolder(folder), mType(type), mActive(active), mPatron(patron) {}

        //! base url
        [[nodiscard]] const String& BaseURL() const { return mBaseUrl; }
        //! Sub-folder
        [[nodiscard]] const String& SubFolder() const { return mFolder; }
        //! Theme type
        [[nodiscard]] ThemeType Type() const { return mType; }
        //! Active ?
        [[nodiscard]] bool Active() const { return mActive; }
        //! Patron ?
        [[nodiscard]] bool Patron() const { return mPatron; }

      private:
        //! Base url
        String mBaseUrl;
        //! Sub folder
        String mFolder;
        //! Type
        ThemeType mType;
        //! Active ?
        bool mActive;
        //! Patreon ?
        bool mPatron;
    };

    /*!
     * @brief Single theme descriptor
     */
    struct ThemeDescriptor
    {
      ThemeDescriptor(const String& baseUrl, const String& installationFolder, const String& content, ThemeType type, bool patron);

      struct Screenshots
      {
        ThemeData::Compatibility mCompatibility; //!< Screenshot compatibility
        ThemeData::Resolutions mResolutions;     //!< Screenshot resolution (only one)
        std::vector<String> mSystems;            //!< Full Path to system list screenshots
        std::vector<String> mGamelists;          //!< Full Path to game list screenshots
        std::vector<String> mMenus;              //!< Full Path to menu screenshots
        std::vector<String> mGameclips;          //!< Full Path to game clip screenshots
      };

      struct File
      {
        String mFiles;                           //!< Theme file (zip)
        ThemeData::Compatibility mCompatibility; //!< Supported compatibilities
        ThemeData::Resolutions mResolutions;     //!< Supported resolutions
        std::vector<Screenshots> mScreenshots;   //!< Screenshot lists
      };

      std::vector<File> mFiles;     //!< Theme files & screenshots
      String mInstallFolder;        //!< Local Installation folder
      String mName;                 //!< Name
      String mAuthor;               //!< Author name
      String mDescription;          //!< Description
      int mVersion;                 //!< Version
      float mRecalbox;              //!< Minimum Recalbox version
      int mBestFileSelected;        //!< Best selected file in the available list
      int mBestScreenshotSelected;  //!< Best selected screenshot from the file list
      int mBestFileSelectedZipSize; //!< Zip size in byte
      ThemeType mType;              //!< Theme type
      bool mPatron;                 //!< Patron theme ?
      bool mValid;                  //! Descriptor is valid ?

      //! Compatibility build from all file compatibilities
      [[nodiscard]] ThemeData::Compatibility CompositeCompatibility() const
      {
        ThemeData::Compatibility result = ThemeData::Compatibility::None;
        for(const File& file : mFiles) result |= file.mCompatibility;
        return result;
      }
      //! Resolution build from all file resolution
      [[nodiscard]] ThemeData::Resolutions CompositeResolution() const
      {
        ThemeData::Resolutions result = ThemeData::Resolutions::None;
        for(const File& file : mFiles) result |= file.mResolutions;
        return result;
      }
    };

    //! Main Grid
    ComponentGrid mGrid;

    std::shared_ptr<TextScrollComponent> mTitle;
    //! Theme text and number
    std::shared_ptr<TextScrollComponent> mThemeName;

    //! Picture slider
    std::shared_ptr<PictureGalery2DComponent> mPictures;

    //! HDMI Icon (display)
    std::shared_ptr<PictureComponent> mHDMI;
    //! CRT Icon (display)
    std::shared_ptr<PictureComponent> mCRT;
    //! JAMMA Icon (display)
    std::shared_ptr<PictureComponent> mJAMMA;
    //! TATE Icon (display)
    std::shared_ptr<PictureComponent> mTATE;
    //! QVGA Icon (resolution)
    std::shared_ptr<PictureComponent> mQVGA;
    //! VGA Icon (resolution)
    std::shared_ptr<PictureComponent> mVGA;
    //! HD Icon (resolution)
    std::shared_ptr<PictureComponent> mHD;
    //! FHD Icon (resolution)
    std::shared_ptr<PictureComponent> mFHD;

    //! Patreon only logo
    std::shared_ptr<PictureComponent> mPatreonOnly;

    //! Markdown component
    std::shared_ptr<MarkdownText> mDescription;

    //! Global resolver
    IGlobalVariableResolver& mGlobalResolver;

    //! Installation theme download
    GuiWaitLongExecution<int, InstallError>* mGuiInstaller;

    //! Static list validated ?
    static bool sRemoteListValidated;
    //! Unvalidated (raw) theme list
    static std::vector<ThemeListItem> sListItems;
    //! Validated theme list
    static std::vector<ThemeDescriptor> sDescriptors;
    //! Local theme list (installed from URL as key)
    HashMap<Path, InstalledThemes> mInstalledThemes;
    //! Local themes
    std::vector<LocalTheme> mLocalThemes;

    //! Markdown tools
    MarkdownTools mTools;

    //! Initialized ?
    bool mInitialized;

    /*!
     * @brief Populate the UI with image & texts from the first theme
     */
    void PopulateUI();

    /*!
     * @brief Refresh theme texts/images regarding the new select theme
     * @param index Theme index
     */
    void RefreshUI(int index);

    /*!
     * Analyse all theme descriptors available and select the best file to install along with the best
     * screenshots to display
     */
    void AnalyseThemeDescriptors();

    /*!
     * @brief Deserialize theme list
     * @param baseUrl Base url (repo URL)
     * @param content json list
     * @param patron True for a a patreon theme, false otherwise
     */
    void DeserializeThemeList(const String& baseUrl, const String& content, bool patron);

    /*
     * Component
     */

    /*!
     * @brief Fill help list
     * @param help Help list to fill
     * @return True if the method filled the list, false otherwise
     */
    bool CollectHelpItems(Help& help) final;

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) final;

    /*!
     * @brief Called once per frame. Override to implement your own drawings.
     * Call your base::Update() to ensure animation and childrens are updated properly
     * @param deltaTime Elapsed time from the previous frame, in millisecond
     */
    void Update(int deltaTime) final
    {
      (void)deltaTime;
      if (!mInitialized)
      {
        mWindow.pushGui((new GuiWaitLongExecution<bool, int>(mWindow, *this))->Execute(true, String::Empty));
        mInitialized = true;
      }
      Gui::Update(deltaTime);
    }

    /*!
     * @brief Load a list of specification of theme installed in /recalbox/share/themes
     * @return Installed theme list
     */
    const HashMap<Path, InstalledThemes>& UpdateLocalThemeList();

    /*!
     * @brief Analyse local theme list and add manually installed themes
     */
    void AddManuallyInstalledTheme();

    /*!
     * @brief Get installed theme matching the remote theme, if any
     * @param descriptor Remote theme descriptor
     * @return InstalledTheme class pointer to the matching installed theme, or nullptr if no matching has been found
     */
    const InstalledThemes* GetThemeInstalled(const GuiThemeManager::ThemeDescriptor& descriptor);

    /*!
     * @brief Install or update the selected theme
     */
    void InstallOrUpdateSelectedTheme(bool force);

    /*!
     * @brief Removed selected theme
     */
    void RemoveSelectedTheme(bool force);

    /*!
     * @brief Empty upodate folder in boot partition
     */
    static void EmptyUpdateFolder();

    bool SelectedIsCurrentTheme()
    {
      const InstalledThemes* installed = GetThemeInstalled(sDescriptors[mPictures->Index()]);
      return installed != nullptr && installed->InstalledIn() == ThemeManager::Instance().CurrentThemePath();
    }

    /*
     * ILongExecution
     */

    int Execute(GuiWaitLongExecution<bool, int>& from, const bool& parameter) final;

    void Completed(const bool& parameter, const int& result) final;

    /*
     * ILongExecution
     */

    InstallError Execute(GuiWaitLongExecution<int, InstallError>& from, const int& parameter) final;

    void Completed(const int& parameter, const InstallError& result) final;

    /*
     * PictureSliderComponent::IChangeNotifications
     */

    void PictureSliderSlotChanged(int slot, int image) override;

    void PictureSliderImageChanged(int slot, int image) final { (void)slot; (void)image; }

    /*
     * HttpClient::IDownlaod
     */

    void DownloadProgress(const HttpClient& http, long long int currentSize, long long int expectedSize) final;

    /*
     * IThemeSwitchable implementation
     */

    /*!
     * @brief Implementation istruct the Theme switcher to use a specific system theme
     * Returning nullptr means the global theme will be used
     * @return SystemData or nullptr
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return nullptr; }

    /*!
     * @brief Called when theme switch.
     * @param theme New Theme
     * @param refreshOnly True if we need to refresh components only
     */
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

    /*
     * IHttpConfiguration
     */

    /*!
     * @brief Configure the given http client
     * @param url Url
     * @param client Client to configure
     */
    void ConfigureHttpClient(const String& url, HttpClient& client) override;
};
