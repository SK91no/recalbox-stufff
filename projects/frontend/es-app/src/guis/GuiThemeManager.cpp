//
// Created by bkg2k on 30/09/25.
//

#include "GuiThemeManager.h"
#include "network/Networks.h"
#include "utils/network/HttpClient.h"
#include "patreon/PatronInfo.h"
#include "rapidjson/document.h"
#include "Upgrade.h"
#include "utils/Files.h"
#include "guis/GuiMsgBox.h"
#include "utils/Sizes.h"
#include "recalbox/RecalboxSystem.h"
#include "utils/Zip.h"
#include "guis/menus/MenuProvider.h"

/*
 2%                             All 8 spaces takes 12%                                    2%
 /+----------+----------+----------+----------+----------+----------+----------+----------+\  Title size %
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 15% or 11% in tate
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 2%
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 15% or 11% in tate
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 2%
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 15% or 11% in tate
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 2%
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 15% or 11% in tate
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 0.001% or 15% in tate
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ Text size
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ Remaining size
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ||          |          |          |          |          |          |          |          ||
 ++----------+----------+----------+----------+----------+----------+----------+----------++ 2%
 \+----------+----------+----------+----------+----------+----------+----------+----------+/

 */

//! Unvalidated lists
bool GuiThemeManager::sRemoteListValidated = false;
// Unvalidated (raw) theme list
std::vector<GuiThemeManager::ThemeListItem> GuiThemeManager::sListItems;
// Validated theme list
std::vector<GuiThemeManager::ThemeDescriptor> GuiThemeManager::sDescriptors;

GuiThemeManager::GuiThemeManager(WindowManager& window, IGlobalVariableResolver& resolver)
  : Gui(window)
  , mGrid(window, Vector2i(10, 12))
  , mGlobalResolver(resolver)
  , mGuiInstaller(nullptr)
  , mInitialized(false)
{
  addChild(&mGrid);

  mLocalThemes =
  {
    { "recalbox-next", { Path::Empty, Path("/recalbox/share_init/system/.emulationstation/themes/recalbox-next"), -1, true } },
    { "recalbox-240p", { Path::Empty, Path("/recalbox/share_init/system/.emulationstation/themes/recalbox-240p"), -1, Renderer::Instance().Is480pOrLower() } },
    { "recalbox-next-v9", { Path::Empty, Path("/recalbox/share_init/system/.emulationstation/themes/recalbox-next-v9"), -1, !Renderer::Instance().Is480pOrLower() } },
  };

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  float height = Renderer::Instance().DisplayHeightAsFloat() * 0.9f;
  float width = Math::min(height * 1.33333f, Renderer::Instance().DisplayWidthAsFloat() * 0.9f);

  // Window
  if (Renderer::Instance().Is480pOrLower())
  {
    int helpBarTop = mWindow.GetHelpBarArea().Top();
    if (height > helpBarTop - 8) height = helpBarTop - 8; /*8 if the background "overscan" estimation*/
    setSize(width, height);
    setPosition((Renderer::Instance().DisplayWidthAsFloat() - width) / 2, (helpBarTop - height) / 2);
  }
  else
  {
    setSize(width, height);
    setPosition((Renderer::Instance().DisplayWidthAsFloat() - width) / 2,
                (Renderer::Instance().DisplayHeightAsFloat() - height) / 2);
  }

  bool tate = mGlobalResolver.IsTate();

  // Title
  mTitle = std::make_shared<TextScrollComponent>(mWindow, _("THEME MANAGER"), Renderer::Instance().Is480pOrLower() ? menuTheme.Text().font : menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);

  // Theme name/number
  mThemeName = std::make_shared<TextScrollComponent>(mWindow, _("LOADING THEME LIST..."), menuTheme.Text().font, menuTheme.Text().color, ::Alignment::Center);
  if (!Renderer::Instance().Is480pOrLower()) mThemeName->SetBold(true);

  // Icons
  int margin = tate ? (int)(Renderer::Instance().DisplayWidthAsFloat() * 0.01f) : (int)(Renderer::Instance().DisplayHeightAsFloat() * 0.02f);
  mHDMI = std::make_shared<PictureComponent>(window, Path(":/themes/hdmi.svg"), ImgProps::KeepRatio, margin, margin); mHDMI->setColor(menuTheme.Title().color);
  mCRT = std::make_shared<PictureComponent>(window, Path(":/themes/crt.svg"), ImgProps::KeepRatio, margin, margin); mCRT->setColor(menuTheme.Title().color);
  mJAMMA = std::make_shared<PictureComponent>(window, Path(":/themes/jamma.svg"), ImgProps::KeepRatio, margin, margin); mJAMMA->setColor(menuTheme.Title().color);
  mTATE = std::make_shared<PictureComponent>(window, Path(":/themes/tate.svg"), ImgProps::KeepRatio, margin, margin); mTATE->setColor(menuTheme.Title().color);
  mQVGA = std::make_shared<PictureComponent>(window, Path(":/themes/qvga.svg"), ImgProps::KeepRatio, margin, margin); mQVGA->setColor(menuTheme.Title().color);
  mVGA = std::make_shared<PictureComponent>(window, Path(":/themes/vga.svg"), ImgProps::KeepRatio, margin, margin); mVGA->setColor(menuTheme.Title().color);
  mHD = std::make_shared<PictureComponent>(window, Path(":/themes/hd.svg"), ImgProps::KeepRatio, margin, margin); mHD->setColor(menuTheme.Title().color);
  mFHD = std::make_shared<PictureComponent>(window, Path(":/themes/fhd.svg"), ImgProps::KeepRatio, margin, margin); mFHD->setColor(menuTheme.Title().color);

  // Patreon only
  mPatreonOnly = std::make_shared<PictureComponent>(window, Path(":/patreon_only.svg"), ImgProps::KeepRatio, margin, margin); mFHD->setColor(menuTheme.Title().color);
  mPatreonOnly->setColor(0);

  // Picture slider
  mPictures = std::make_shared<PictureGalery2DComponent>(window, ImgProps::KeepRatio, this);
  mPictures->SetBackgroundColor(0xFFFFFF20);

  // Description
  mDescription = std::make_shared<MarkdownText>(window, String::Empty, menuTheme.Text().color, ::Alignment::TopLeft);
  mDescription->SetBackgroundColor(0x00000020)
               .SetPadding(Renderer::Instance().DisplayHeightAsFloat() * 0.02f);

  // Grid
  float coeff = Renderer::Instance().Is480pOrLower() ? 1.8 : 1.5f;
  for(int i = mGrid.Columns() - 1; --i > 0; ) mGrid.setColWidthPerc(i, (1.f - 0.02f * 2) / ((float)mGrid.Columns() - 2));
  mGrid.setColWidthPerc(0, 0.02f);
  mGrid.setColWidthPerc(mGrid.Columns() - 1, 0.02f);
  mGrid.setRowHeightPerc(0, ((float)mTitle->getFont()->Height() * coeff) / Renderer::Instance().DisplayHeightAsFloat());
  mGrid.setRowHeightPerc(1, tate ? 0.12f : 0.15f);
  mGrid.setRowHeightPerc(2, 0.02f);
  mGrid.setRowHeightPerc(3, tate ? 0.12f : 0.15f);
  mGrid.setRowHeightPerc(4, 0.02f);
  mGrid.setRowHeightPerc(5, tate ? 0.12f : 0.15f);
  mGrid.setRowHeightPerc(6, 0.02f);
  mGrid.setRowHeightPerc(7, tate ? 0.12f : 0.15f);
  mGrid.setRowHeightPerc(8, tate ? 0.15f : 0.001f);
  mGrid.setRowHeightPerc(9, ((float)menuTheme.Text().font->Height() * coeff) / Renderer::Instance().DisplayHeightAsFloat());
  mGrid.setRowHeightPerc(10, 0);
  mGrid.setRowHeightPerc(11, 0.02f);
  //mGrid.SetColumnHighlight(true, 0, mGrid.Columns() - 1);
  //mGrid.SetRowHighlight(true, 0, mGrid.Rows() - 1);

  mGrid.setEntry(mTitle, { 1, 0 }, false, true, { 8, 1 });
  mGrid.setEntry(mPictures, { 2, 1 }, true, true, { 6, 7 });
  if (tate)
  {
    mGrid.setEntry(mHDMI, { 1, 8 }, false, true);
    mGrid.setEntry(mCRT, { 2, 8 }, false, true);
    mGrid.setEntry(mJAMMA, { 3, 8 }, false, true);
    mGrid.setEntry(mTATE, { 4, 8 }, false, true);
    mGrid.setEntry(mQVGA, { 5, 8 }, false, true);
    mGrid.setEntry(mVGA, { 6, 8 }, false, true);
    mGrid.setEntry(mHD, { 7, 8 }, false, true);
    mGrid.setEntry(mFHD, { 8, 8 }, false, true);
  }
  else
  {
    mGrid.setEntry(mHDMI, { 1, 1 }, false, true);
    mGrid.setEntry(mCRT, { 1, 3 }, false, true);
    mGrid.setEntry(mJAMMA, { 1, 5 }, false, true);
    mGrid.setEntry(mTATE, { 1, 7 }, false, true);
    mGrid.setEntry(mQVGA, { 8, 1 }, false, true);
    mGrid.setEntry(mVGA, { 8, 3 }, false, true);
    mGrid.setEntry(mHD, { 8, 5 }, false, true);
    mGrid.setEntry(mFHD, { 8, 7 }, false, true);
  }
  mGrid.setEntry(mThemeName, { 1, 9 }, false, true, { 8, 1 });
  mGrid.setEntry(mDescription, { 1, 10 }, false, true, { 8, 1 });
  mGrid.setEntry(mPatreonOnly, { 7, 1 }, false, true, { 1, 2});

  mGrid.setSize(width, height);

  TextureManager::Instance().RegisterHttpFilter(this);
}

GuiThemeManager::~GuiThemeManager()
{
  TextureManager::Instance().UnregisterHttpFilter(this);
}

bool GuiThemeManager::ProcessInput(const InputCompactEvent& event)
{
  if (event.ValidReleased()) InstallOrUpdateSelectedTheme(false);
  else if (event.NorthReleased()) RemoveSelectedTheme(false);
  else if (event.WestReleased() && SelectedIsCurrentTheme())
  {
    // force hasdefaulttransition check to grey unneeded options
    ThemeManager::Instance().Main().getThemeTransition();
    MenuProvider::ShowMenu(MenuContainerType::ThemeOptions, InheritableContext());
  }
  else if (event.StartReleased()) { sListItems.clear(); sDescriptors.clear(); mInitialized = false; }
  else if (event.CancelReleased()) Close();

  return mPictures->ProcessInput(event);
}

int GuiThemeManager::Execute(GuiWaitLongExecution<bool, int>& from, const bool& parameter)
{
  (void)from;
  (void)parameter;

  from.SetText(_("Loading theme list..."));
  if (!sRemoteListValidated)
  {
    sListItems.clear();
    sDescriptors.clear();
    sRemoteListValidated = true;

    // Prepare http request
    HttpClient http;
    bool IsPatron = PatronInfo::Instance().IsPatron();
    if (IsPatron)
      http.SetBearer(PatronInfo::Instance().Token());

    // Get local themes
    Path folder("/recalbox/system/themes");
    for(const LocalTheme& local : mLocalThemes)
      sListItems.push_back({folder.ToString(), local.mDescriptorFolder, true, ThemeType::Embedded, false });

    // Get public themes
    String publicHubUrl = Networks::QueryDNSRecord(sThemeHubDomain);
    if (publicHubUrl.empty()) { LOG(LogError) << "[GuiThemeManager] Error getting public theme url."; sRemoteListValidated = false; }
    else
    {
      String url(publicHubUrl); url.Append("/-/raw/main/list.json");
      if (String content; http.Execute(url, content, false, 3)) DeserializeThemeList(publicHubUrl, content, false);
      else { LOG(LogError) << "[GuiThemeManager] Error loading public theme list."; sRemoteListValidated = false; }
    }

    // get patron themes
    if (IsPatron)
    {
      String privateHubUrl = Networks::QueryDNSRecord(sPatronThemeHubDomain);
      if (privateHubUrl.empty()) { LOG(LogError) << "[GuiThemeManager] Error getting private theme url."; sRemoteListValidated = false; }
      String url(privateHubUrl); url.Append("/-/raw/main/list.json");
      if (String content; http.Execute(url, content, false, 3)) DeserializeThemeList(privateHubUrl, content, true);
      else { LOG(LogError) << "[GuiThemeManager] Error loading Patron theme list !"; sRemoteListValidated = false; }
    }

    int index = 0;
    for(const ThemeListItem& item : sListItems)
      if (item.Active())
      {
        from.SetSecondaryText(_("Loading theme information..."));
        from.SetProgress((++index * 100) / (int)sListItems.size());

        String baseUrl(item.BaseURL()); baseUrl.Append('/').Append(item.SubFolder());
        String url(baseUrl); url.Append("/descriptor.json");
        String content;
        if (url.StartsWith("http://") || url.StartsWith("https://"))
        {
          if (!http.Execute(url, content, false, 3))
          { LOG(LogError) << "[GuiThemeManager] Error downloading descriptor " << url; sRemoteListValidated = false; continue; }
        }
        else content = Files::LoadFile(Path(url));
        if (ThemeDescriptor descriptor = ThemeDescriptor(baseUrl, item.SubFolder(), content, item.Type(), item.Patron()); descriptor.mValid)
          sDescriptors.push_back(descriptor);
      }
  }

  // Read installed themes
  UpdateLocalThemeList();
  // Analyse manually installed themes
  AddManuallyInstalledTheme();
  // Analyse themes data & select best screenshot and theme files
  AnalyseThemeDescriptors();

  return (int)sDescriptors.size();
}

void GuiThemeManager::Completed(const bool& parameter, const int& result)
{
  (void)parameter;
  if (result <= 0)
  {
    { LOG(LogError) << "[GuiThemeManager] Error loading theme lists. Error code: " << result; }
    mWindow.displayMessage(_("Unexpected error while retrieving theme list ! Please retry later."));
    Close();
  }
  else
  {
    // Populate picture slider component & set curren theme
    PopulateUI();
  }
}

void GuiThemeManager::DeserializeThemeList(const String& baseUrl, const String& content, bool patron)
{
  rapidjson::Document json;
  json.Parse(content.c_str());
  if (json.HasParseError()) { LOG(LogError) << "[GuiThemeManager] Error parsing theme lists from: " << baseUrl; return; }
  if (!json.HasMember("themes")) { LOG(LogError) << "[GuiThemeManager] Error parsing theme lists from: " << baseUrl; return; }
  for(const auto& theme : json["themes"].GetArray())
  {
    if (!theme.HasMember("folder")) { LOG(LogError) << "[GuiThemeManager] Missing older entry in theme lists from: " << baseUrl; continue; }
    sListItems.push_back({ String(baseUrl).Append("/-/raw/main"), theme["folder"].GetString(), theme.HasMember("active") && theme["active"].IsBool() ? theme["active"].GetBool() : true, ThemeType::Remote, patron });
  }
}

void GuiThemeManager::PopulateUI()
{
  // Populate screenshot urls
  mPictures->Clear();
  for(int i = 0; i < (int)sDescriptors.size(); ++i)
  {
    ThemeDescriptor& descriptor = sDescriptors[i];
    ThemeDescriptor::File& file = descriptor.mFiles[descriptor.mBestFileSelected];
    ThemeDescriptor::Screenshots screenshots = file.mScreenshots[descriptor.mBestScreenshotSelected];
    { LOG(LogDebug) << "[GuiThemeManager] Theme " << descriptor.mName << " best file selected: " << descriptor.mBestFileSelected << " - Best screenshot slot selected: " << descriptor.mBestScreenshotSelected; }
    std::vector<Path> allScreenshots;
    for(const String& url : screenshots.mSystems  ) allScreenshots.push_back(Path(url));
    for(const String& url : screenshots.mGamelists) allScreenshots.push_back(Path(url));
    for(const String& url : screenshots.mMenus    ) allScreenshots.push_back(Path(url));
    for(const String& url : screenshots.mGameclips) allScreenshots.push_back(Path(url));
    int screenshotIndex = -1;
    for(const Path& path : allScreenshots)
    {
      mPictures->setImage(i, ++screenshotIndex, path);
      { LOG(LogDebug) << "[GuiThemeManager]   ScreenshotPath #" << i << '/' << screenshotIndex << " : " << path; }
    }
  }

  // Theme position
  int index = 0;
  int currentIndex = 0;
  for(const ThemeDescriptor& descriptor : sDescriptors)
  {
    if (const InstalledThemes* installed = GetThemeInstalled(descriptor); installed != nullptr)
      if (installed->InstalledIn() == ThemeManager::Instance().CurrentThemePath())
      {
        currentIndex = index;
        break;
      }
    ++index;
  }

  // Refresh first theme data
  mPictures->SetIndex(currentIndex, false);
  RefreshUI(currentIndex);
}

void GuiThemeManager::AddManuallyInstalledTheme()
{
  String content;
  Path descriptorPath("/recalbox/system/themes/manually-template");
  for(const auto& it : mInstalledThemes)
  {
    String localSubFolder = it.first.Filename();
    // Lookup current them in remote list
    bool remoteFound = false;
    for(const ThemeDescriptor& descriptor : sDescriptors)
      if (descriptor.mInstallFolder == localSubFolder) { remoteFound = true; break; }
    if (remoteFound) continue;

    // Create a new local theme
    if (content.empty()) content = Files::LoadFile(descriptorPath / "descriptor.json");
    if (ThemeDescriptor descriptor = ThemeDescriptor(descriptorPath.ToString(), localSubFolder, content, ThemeType::ManuallyInstalled, false); descriptor.mValid)
    {
      ThemeData::Compatibility compatibility = ThemeData::Compatibility::None;
      ThemeData::Resolutions resolutions = ThemeData::Resolutions::None;
      String name;
      String author;
      int version = 0;
      int recalboxVersion = 0;
      ThemeData::FetchCompatibility(it.first / ThemeManager::sRootThemeFile, compatibility, resolutions, name, author, version, recalboxVersion);
      if (name.empty()) name = localSubFolder;
      if (author.empty()) author = _("UNKNOWN");
      descriptor.mName = name;
      descriptor.mAuthor = author;
      descriptor.mVersion = version;
      descriptor.mRecalbox = (float)recalboxVersion / 256.f;
      sDescriptors.push_back(descriptor);
    }
  }
}

void GuiThemeManager::RefreshUI(int index)
{
  ThemeDescriptor& descriptor = sDescriptors[index];

  // Refresh theme name & number
  mTitle->setText(_("THEME MANAGER").Append(' ').Append('(').Append(index + 1).Append('/').Append((int)sDescriptors.size()).Append(')'));
  mThemeName->setText(String(descriptor.mName).Append(" [", 2).Append(GetThemeInstalled(descriptor) != nullptr ? _("Installed") : _("Not Installed")).Append(']'));

  // Refresh icon
  unsigned int compatibleColor = ThemeManager::Instance().Menu().Title().color;
  unsigned int notCompatibleColor = (compatibleColor & 0xFFFFFF00) | 0x40;
  ThemeData::Compatibility compatibility = descriptor.CompositeCompatibility();
  ThemeData::Resolutions resolutions = descriptor.CompositeResolution();
  mHDMI->setColor(hasFlag(compatibility, ThemeData::Compatibility::Hdmi) ? compatibleColor : notCompatibleColor);
  mCRT->setColor(hasFlag(compatibility, ThemeData::Compatibility::Crt) ? compatibleColor : notCompatibleColor);
  mJAMMA->setColor(hasFlag(compatibility, ThemeData::Compatibility::Jamma) ? compatibleColor : notCompatibleColor);
  mTATE->setColor(hasFlag(compatibility, ThemeData::Compatibility::Tate) ? compatibleColor : notCompatibleColor);
  mQVGA->setColor(hasFlag(resolutions, ThemeData::Resolutions::QVGA) ? compatibleColor : notCompatibleColor);
  mVGA->setColor(hasFlag(resolutions, ThemeData::Resolutions::VGA) ? compatibleColor : notCompatibleColor);
  mHD->setColor(hasFlag(resolutions, ThemeData::Resolutions::HD) ? compatibleColor : notCompatibleColor);
  mFHD->setColor(hasFlag(resolutions, ThemeData::Resolutions::FHD) ? compatibleColor : notCompatibleColor);
  mPatreonOnly->setColor(descriptor.mPatron ? compatibleColor : 0);

  // Refresh markdown informations
  mTools.SetEnabled(!Renderer::Instance().Is480pOrLower());
  String md(mTools.Bold(_("Author")));
  md.Append(':').Append(' ').Append(descriptor.mAuthor).Append(String::CRLF);
  if (descriptor.mVersion > 0) md.Append(mTools.Bold(_("Version"))).Append(':').Append(' ').Append(descriptor.mVersion >> 8).Append('.').Append(descriptor.mVersion & 0xFF).Append(String::CRLF);
  else md.Append(mTools.Bold(_("Version"))).Append(':').Append(' ').Append(_("UNKNOWN")).Append(String::CRLF);
  if (descriptor.mRecalbox > 0) md.Append(mTools.Bold(_("Compatible with"))).Append(':').Append(LEGACY_STRING(" Recalbox v")).Append(descriptor.mRecalbox).Append(' ').Append(_("and later versions")).Append(String::CRLF);
  else  md.Append(mTools.Bold(_("Compatible with"))).Append(':').Append(' ').Append(_("UNKNOWN")).Append(String::CRLF);
  if (descriptor.mBestFileSelectedZipSize > 0)
    md.Append(mTools.Bold(_("Download Size"))).Append(':').Append(' ').Append(Sizes(descriptor.mBestFileSelectedZipSize).ToHumanSize()).Append(String::CRLF);
  md.Append(mTools.Bold(_("Description"))).Append(':').Append(' ').Append(descriptor.mDescription);
  mDescription->SetText(md);

  mWindow.UpdateHelpSystem();
}

void GuiThemeManager::AnalyseThemeDescriptors()
{
  // Get current specs
  bool tate = mGlobalResolver.IsTate();
  ThemeData::Compatibility currentCompatibility = (mGlobalResolver.HasJamma() ? ThemeData::Compatibility::Jamma : ThemeData::Compatibility::None) |
                                                  (mGlobalResolver.HasCrt() ? ThemeData::Compatibility::Crt : ThemeData::Compatibility::None) |
                                                  (mGlobalResolver.HasHDMI() ? ThemeData::Compatibility::Hdmi : ThemeData::Compatibility::None);
  ThemeData::Resolutions currentResolution = mGlobalResolver.IsQVGA() ? ThemeData::Resolutions::QVGA :
                                             mGlobalResolver.IsVGA() ? ThemeData::Resolutions::VGA :
                                             mGlobalResolver.IsHD() ? ThemeData::Resolutions::HD :
                                             ThemeData::Resolutions::FHD;

  // Run though valid descriptors
  HttpClient http;
  if (PatronInfo::Instance().IsPatron()) http.SetBearer(PatronInfo::Instance().Token());
  for(ThemeDescriptor& descriptor : sDescriptors)
  {
    int fileIndex = 0;
    int screenshotIndex = 0;

    // Seek for the best file & best screenshots
    int fileBestTateDistance = 256;
    int fileBestCompatibilityDistance = 256;
    int fileBestResolutionDistance = 256;
    for(int findex = (int)descriptor.mFiles.size(); --findex >= 0; )
    {
      const ThemeDescriptor::File& file = descriptor.mFiles[findex];

      // Check tate distance, the most discriminant value
      if (int tateDistance = Math::absi((int)tate - (int) hasFlag(file.mCompatibility, ThemeData::Compatibility::Tate)); tateDistance <= fileBestTateDistance) fileBestTateDistance = tateDistance;
      else continue;

      // Then check the display
      static constexpr ThemeData::Compatibility compatibilities[] { ThemeData::Compatibility::Crt, ThemeData::Compatibility::Jamma, ThemeData::Compatibility::Hdmi };
      bool found = false;
      for(ThemeData::Compatibility comp : compatibilities)
        if ((comp & file.mCompatibility) == comp)
          if (int compDistance = Math::absi((int)comp - (int)currentCompatibility); compDistance <= fileBestCompatibilityDistance) { found = true; fileBestCompatibilityDistance = compDistance; }
      if (!found) continue;

      // Then check the resolution
      static constexpr ThemeData::Resolutions resolutions[] { ThemeData::Resolutions::QVGA, ThemeData::Resolutions::VGA, ThemeData::Resolutions::HD, ThemeData::Resolutions::FHD };
      found = false;
      for(ThemeData::Resolutions res : resolutions)
        if ((res & file.mResolutions) == res)
          if (int resDistance = Math::absi((int)res - (int)currentResolution); resDistance <= fileBestResolutionDistance) { found = true; fileBestResolutionDistance = resDistance; }
      if (!found) continue;

      // Record current file
      fileIndex = findex;

      int screenshotBestTateDistance = 256;
      int screenshotBestCompatibilityDistance = 256;
      int screenshotBestResolutionDistance = 256;
      for(int sindex = (int)file.mScreenshots.size(); --sindex >= 0; )
      {
        const ThemeDescriptor::Screenshots& screenshot = file.mScreenshots[sindex];

        // Check tate distance, the most discriminant value
        if (int tateDistance = Math::absi((int)tate - (int) hasFlag(screenshot.mCompatibility, ThemeData::Compatibility::Tate)); tateDistance <= screenshotBestTateDistance) screenshotBestTateDistance = tateDistance;
        else continue;

        // Then check the display
        found = false;
        for(ThemeData::Compatibility comp : compatibilities)
          if ((comp & screenshot.mCompatibility) == comp)
            if (int compDistance = Math::absi((int)comp - (int)currentCompatibility); compDistance <= screenshotBestCompatibilityDistance) { found = true; screenshotBestCompatibilityDistance = compDistance; }
        if (!found) continue;

        // Then check the resolution
        found = false;
        for(ThemeData::Resolutions res : resolutions)
          if ((res & screenshot.mResolutions) == res)
            if (int resDistance = Math::absi((int)res - (int)currentResolution); resDistance <= screenshotBestResolutionDistance) { found = true; screenshotBestResolutionDistance = resDistance; }
        if (!found) continue;

        // Record Screenshot
        screenshotIndex = sindex;
      }
    }

    // Record best file in current descriptor
    descriptor.mBestFileSelected = fileIndex;
    descriptor.mBestFileSelectedZipSize = -1;
    String themePath = descriptor.mFiles[descriptor.mBestFileSelected].mFiles;
    if (themePath.StartsWith("http://") || themePath.StartsWith("https://"))
      http.GetFileSize(descriptor.mFiles[descriptor.mBestFileSelected].mFiles, descriptor.mBestFileSelectedZipSize);
    descriptor.mBestScreenshotSelected = screenshotIndex;
  }
}

bool GuiThemeManager::CollectHelpItems(Help& help)
{
  const InstalledThemes* installed = nullptr;
  bool updateAvailable = false;
  bool local = false;
  if (!sDescriptors.empty())
  {
    ThemeDescriptor& selectedTheme = sDescriptors[mPictures->Index()];
    local = selectedTheme.mType == ThemeType::Embedded;
    installed = GetThemeInstalled(selectedTheme);
    updateAvailable = installed != nullptr && installed->Version() >= 0 && installed->Version() < selectedTheme.mVersion;
  }

  help.Set(HelpType::UpDown, _("BROWSE PREVIEWS"))
      .Set(HelpType::LeftRight, _("BROWSE THEMES"))
      .Set(Help::Cancel(), _("CLOSE"));
  if (installed == nullptr) help.Set(Help::Valid(), _("INSTALL"));
  else
  {
    if (!local) help.Set(HelpType::X, _("DELETE"));
    if (installed->InstalledIn() == ThemeManager::Instance().CurrentThemePath())
      help.Set(HelpType::Y, _("OPTIONS"));
    if (updateAvailable) help.Set(Help::Valid(), _("UPDATE"));
    else                 help.Set(Help::Valid(), _("SWITCH TO"));
  }
  return true;
}

void GuiThemeManager::PictureSliderSlotChanged(int slot, int image)
{
  (void)image;
  RefreshUI(slot);
}

const HashMap<Path, GuiThemeManager::InstalledThemes>& GuiThemeManager::UpdateLocalThemeList()
{
  // Get theme list
  ThemeManager::ThemeList themelist = ThemeManager::AvailableThemes();

  String currentVersionString = Upgrade::CurrentVersion();
  int cut = (int)currentVersionString.find_first_not_of("0123456789.");
  if (cut >= 0) currentVersionString.Delete(cut, INT32_MAX);

  mInstalledThemes.clear();
  for (const auto& theme : themelist)
    if (theme.second.StartWidth(sLocalThemeFolder))
    {
      String displayableName;
      int version = 0;
      bool compatible = ThemeManager::Instance().AnalyseAndSwitch(mWindow, theme.second, displayableName, false, &version);
      Path url(Files::LoadFile(theme.second / sInstalledFromFileName));
      mInstalledThemes.insert_or_assign(theme.second, { url, theme.second, version, compatible });
    }

  return mInstalledThemes;
}

const GuiThemeManager::InstalledThemes* GuiThemeManager::GetThemeInstalled(const GuiThemeManager::ThemeDescriptor& descriptor)
{
  switch(descriptor.mType)
  {
    case ThemeType::Embedded:
    {
      for(const LocalTheme& installed : mLocalThemes)
        if (installed.mInstalled.InstalledIn().Filename() == descriptor.mInstallFolder)
          return &installed.mInstalled;
      break;
    }
    case ThemeType::ManuallyInstalled:
    {
      String localSubFolder(descriptor.mInstallFolder);
      Path localPath = Path(sLocalThemeFolder) / localSubFolder;
      for(const auto& installed : mInstalledThemes)
        if (installed.second.InstalledIn() == localPath)
          return &installed.second;
      break;
    }
    case ThemeType::Remote:
    {
      // Build local theme path
      String localSubFolder(descriptor.mInstallFolder);
      Path localPath = Path(sLocalThemeFolder) / localSubFolder;
      if (!localPath.Exists()) return nullptr;

      // Load local url file
      Path urlFrom(Files::LoadFile(localPath / sInstalledFromFileName));
      for(const auto& installed : mInstalledThemes)
        if (installed.second.InstalledFrom() == urlFrom)
          return &installed.second;
      break;
    }
  }

  return nullptr;
}

void GuiThemeManager::InstallOrUpdateSelectedTheme(bool force)
{
  if (sDescriptors.empty()) return;

  ThemeDescriptor& selectedTheme = sDescriptors[mPictures->Index()];
  const InstalledThemes* installed = GetThemeInstalled(selectedTheme);

  // Switch on installed theme
  if (installed != nullptr && (installed->Version() < 0 || installed->Version() >= selectedTheme.mVersion))
  {
    String dummy;
    ThemeManager::Instance().AnalyseAndSwitch(mWindow, installed->InstalledIn(), dummy, true);
    RefreshUI(mPictures->Index());
    return;
  }

  // Install/update
  if (!force)
  {
    String text = installed != nullptr && installed->Version() < selectedTheme.mVersion ?
                  _("Please confirm. Do you want to update the theme {0}?") :
                  _("Please confirm. Do you want to install the theme {0}?");
    mWindow.pushGui((new GuiMsgBox(mWindow, (_F(text) / selectedTheme.mName).ToString(), _("NO"), nullptr, _("YES"), [this] { InstallOrUpdateSelectedTheme(true); }))->SetDefaultButton(1));
  }
  else
    mWindow.pushGui((new GuiWaitLongExecution<int, InstallError>(mWindow, *this))->Execute(mPictures->Index(), String::Empty));
}

void GuiThemeManager::RemoveSelectedTheme(bool force)
{
  if (sDescriptors.empty()) return;

  ThemeDescriptor& selectedTheme = sDescriptors[mPictures->Index()];
  const InstalledThemes* installed = GetThemeInstalled(selectedTheme);
  if (installed == nullptr) return;

  if (!force)
    mWindow.pushGui(new GuiMsgBox(mWindow, (_F(_("Please confirm. Do you really want to delete the theme {0}?")) / selectedTheme.mName).ToString(), _("NO"), nullptr, _("YES"), [this] { RemoveSelectedTheme(true); }));
  else
  {
    bool isCurrentTheme = ThemeManager::Instance().CurrentThemePath() == installed->InstalledIn();
    if (!installed->InstalledIn().RecurseDelete()) { LOG(LogError) << "[GuiThemeManager] Cannot delete file(s) in " << installed->InstalledIn(); }
    (void)installed->InstalledIn().Delete();
    if (String dummy; isCurrentTheme) ThemeManager::Instance().AnalyseAndSwitch(mWindow, Path("/recalbox/share_init/system/.emulationstation/themes") / ThemeManager::sRootThemeFile, dummy, true);
    UpdateLocalThemeList();
    RefreshUI(mPictures->Index());
  }
}

InstallError GuiThemeManager::Execute(GuiWaitLongExecution<int, InstallError>& from, const int& parameter)
{
  InstallError result = InstallError::Ok;

  from.SetText(_("Preparing theme installation..."));
  EmptyUpdateFolder();
  RecalboxSystem::MakeBootReadWrite();

  const ThemeDescriptor& descriptor = sDescriptors[parameter];

  from.SetText((_F(_("Downloading theme {0}")) / descriptor.mName).ToString());
  HttpClient http;
  if (PatronInfo::Instance().IsPatron()) http.SetBearer(PatronInfo::Instance().Token());
  Path tmpZip = Path(sInstallationTmpFolder) / "theme.zip";
  mGuiInstaller = &from;
  if (http.Execute(descriptor.mFiles[descriptor.mBestFileSelected].mFiles, tmpZip, this, true, 10))
  {
    String localSubFolder(descriptor.mInstallFolder);
    String localTmpSubFolder(localSubFolder); localTmpSubFolder.Append(".tmp");
    String localOldSubFolder(localSubFolder); localOldSubFolder.Append(".old");
    Path localPath = Path(sLocalThemeFolder) / localTmpSubFolder;
    Path localPathOld = Path(sLocalThemeFolder) / localOldSubFolder;
    Path localPathFinal = Path(sLocalThemeFolder) / localSubFolder;

    from.SetText((_F(_("Installing theme {0}")) / descriptor.mName).ToString());
    Zip zip(tmpZip);
    //! Extract main theme.xml path
    Path shortestThemeXmlPath;
    for(int i = zip.Count(); --i >= 0; )
      if (Path s = zip.FileName(i); (shortestThemeXmlPath.IsEmpty() || s.ToString().Count() < shortestThemeXmlPath.ToString().Count()) && s.ToString().EndsWith("/theme.xml"))
        shortestThemeXmlPath = s;
    shortestThemeXmlPath = shortestThemeXmlPath.Directory();
    bool ok = false;
    int progress = 0;
    if (localPath.CreatePath())
    {
      // Unzip all
      for (int i = zip.Count(); --i >= 0;)
      {
        Path path = localPath / zip.FileName(i).MakeRelative(shortestThemeXmlPath, ok);
        String content = zip.Content(i);
        if (Path dir(path.Directory()); !dir.Exists()) (void)dir.CreatePath();
        if (!content.empty())
          if (!Files::SaveFile(path, content))
          {
            { LOG(LogError) << "[GuiThemeManager] Error unzipping " << descriptor.mFiles[descriptor.mBestFileSelected].mFiles; }
            result = InstallError::WriteFailed;
            break;
          }
        mGuiInstaller->SetSecondaryText((_F(_("Installed {0}%")) / ((++progress * 100) / zip.Count())).ToString());
        from.SetProgress((progress * 100) / zip.Count());
        Thread::Sleep(0);
      }
      if (result == InstallError::Ok) // Still ok ?
      {
        // Rename
        mGuiInstaller->SetSecondaryText(_("Cleaning..."));
        ok = !localPathOld.Exists() || (localPathOld.RecurseDelete() && localPathOld.Delete());
        if (ok) ok = !localPathFinal.Exists() || Path::Rename(localPathFinal, localPathOld);
        if (ok) ok = Path::Rename(localPath, localPathFinal);
        if (ok) ok = !localPathOld.Exists() || (localPathOld.RecurseDelete() && localPathOld.Delete());
        if (ok) ok = Files::SaveFile(localPathFinal / sInstalledFromFileName, descriptor.mFiles[descriptor.mBestFileSelected].mFiles);
        if (!ok)
        {
          result = InstallError::RenameError;
          { LOG(LogError) << "[GuiThemeManager] Error renaming " << descriptor.mFiles[descriptor.mBestFileSelected].mFiles; }
        }
      }
      else // Cleanup on error
      {
        (void)localPath.RecurseDelete();
        (void)localPath.Delete();
        { LOG(LogError) << "[GuiThemeManager] Deleting " << localPath; }
      }
    }
    else
    {
      { LOG(LogError) << "[GuiThemeManager] Error unzipping " << descriptor.mFiles[descriptor.mBestFileSelected].mFiles; }
      result = InstallError::WriteFailed;
    }
  }
  else
  {
    { LOG(LogError) << "[GuiThemeManager] Error downloading " << descriptor.mFiles[descriptor.mBestFileSelected].mFiles; }
    result = InstallError::DownloadFailed;
  }

  // Cleanup
  mGuiInstaller = nullptr;
  (void)tmpZip.Delete();
  RecalboxSystem::MakeBootReadOnly();

  return result;
}

void GuiThemeManager::Completed(const int& parameter, const InstallError& result)
{
  const ThemeDescriptor& descriptor = sDescriptors[parameter];
  String errorMessage((_F(_("Error installing theme {0}\nReason: {1}"))).ToString());

  switch(result)
  {
    case InstallError::Ok:
    {
      Path localPath = Path(sLocalThemeFolder) / descriptor.mInstallFolder;
      UpdateLocalThemeList();
      RefreshUI(mPictures->Index());
      mWindow.pushGui(new GuiMsgBox(mWindow, (_F(_("Do you want to switch to the newly installed theme {0} ?")) / descriptor.mName).ToString(),
                                    _("YES"), [parameter, this] { String dummy; ThemeManager::Instance().AnalyseAndSwitch(mWindow, Path(sLocalThemeFolder) / sDescriptors[parameter].mInstallFolder, dummy, true); },
                                    _("NO"), nullptr));
      break;
    }
    case InstallError::DownloadFailed: mWindow.displayMessage((_F(errorMessage) / descriptor.mName / _("Failed to download theme file. Please check your internet connection.")).ToString()); break;
    case InstallError::WriteFailed:
    case InstallError::RenameError: mWindow.displayMessage((_F(errorMessage) / descriptor.mName / _("Failed to install required files. Please check you've enough free space on your storage !")).ToString()); break;
    default: mWindow.displayMessage((_F(errorMessage) / descriptor.mName / _("Unknown failure.")).ToString()); break;
  }
}

void GuiThemeManager::EmptyUpdateFolder()
{
  RecalboxSystem::EmptyUpdateFolder();
}

void GuiThemeManager::DownloadProgress(const HttpClient& http, long long int currentSize, long long int expectedSize)
{
  (void)http;
  if (mGuiInstaller != nullptr)
  {
    mGuiInstaller->SetSecondaryText((_F(_("Downloaded {0}%")) / (int)((currentSize * 100) / expectedSize)).ToString());
    mGuiInstaller->SetProgress((int)((currentSize * 100) / expectedSize));
  }
}

void GuiThemeManager::SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface)
{
  (void)theme;
  (void)refreshOnly;
  (void)interface;
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  // Title
  mTitle->setFont(Renderer::Instance().Is480pOrLower() ? menuTheme.Text().font : menuTheme.Title().font);
  mTitle->setColor(menuTheme.Title().color);

  // Theme name/number
  mThemeName->setFont(menuTheme.Text().font);
  mThemeName->setColor(menuTheme.Text().color);
  mThemeName->SetBold(true);

  // Description
  mDescription->SetColor(menuTheme.Text().color);
  mDescription->SetFont(menuTheme.Text().font);

  // Grid
  mGrid.setRowHeightPerc(0, ((float)mTitle->getFont()->Height() * 1.5f) / Renderer::Instance().DisplayHeightAsFloat());
  mGrid.setRowHeightPerc(9, ((float)menuTheme.Text().font->Height() * 1.5f) / Renderer::Instance().DisplayHeightAsFloat());
}

void GuiThemeManager::ConfigureHttpClient(const String& url, HttpClient& client)
{
  // Configure bearer for inner domains
  if (url.Contains(".recalbox.com"))
    if (PatronInfo::Instance().IsPatron()) client.SetBearer(PatronInfo::Instance().Token());
}

GuiThemeManager::ThemeDescriptor::ThemeDescriptor(const String& baseUrl, const String& installationFolder, const String& content, ThemeType type, bool patron)
  : mValid(false)
{
  String language("en");
  String lccc = RecalboxConf::Instance().GetSystemLanguage().LowerCase();
  if (lccc.size() >= 5)
    if (int pos = lccc.Find('_'); pos >= 2 && pos < (int) lccc.size() - 1)
      language = lccc.SubString(0, pos);

  rapidjson::Document json;
  json.Parse(content.c_str());
  if (json.HasParseError()) { LOG(LogError) << "[GuiThemeManager] Error parsing descriptor from: " << baseUrl; return; }
  // Check global info validity
  if (!json.HasMember("name") || !json["name"].IsString()) { LOG(LogError) << "[GuiThemeManager] Missing name or incorrect type in descriptor from: " << baseUrl; return; }
  if (!json.HasMember("version") || (!json["version"].IsFloat() && !json["version"].IsInt())) { LOG(LogError) << "[GuiThemeManager] Missing version or incorrect type in descriptor from: " << baseUrl; return; }
  if (!json.HasMember("recalbox-version") || (!json["recalbox-version"].IsFloat() && !json["recalbox-version"].IsInt())) { LOG(LogError) << "[GuiThemeManager] Missing recalbox version or incorrect type in descriptor from: " << baseUrl; return; }
  if (!json.HasMember("author") || !json["author"].IsString()) { LOG(LogError) << "[GuiThemeManager] Missing author or incorrect type in descriptor from: " << baseUrl; return; }
  if (!json.HasMember("files") || !json["files"].IsArray()) { LOG(LogError) << "[GuiThemeManager] Missing file array or incorrect type in descriptor from: " << baseUrl; return; }
  // Fill in global information
  mPatron = patron;
  mInstallFolder = installationFolder;
  mName = json["name"].GetString();
  String version(json["version"].GetDouble()); version.TrimRight('0');
  if (String major, minor; version.Extract('.', major, minor, true)) mVersion = (major.AsInt() << 8) + minor.AsInt();
  else                                                               mVersion = version.AsInt() << 8;
  mRecalbox = json["recalbox-version"].GetFloat();
  mAuthor = json["author"].GetString();
  if (String s = String("description.").Append(lccc); json.HasMember(s.c_str())) mDescription = json[s.c_str()].GetString();
  else if (s = String("description.").Append(language); json.HasMember(s.c_str())) mDescription = json[s.c_str()].GetString();
  else mDescription = json["description"].GetString();
  mType = type;
  mBestFileSelected = 0;
  mBestScreenshotSelected = 0;

  // Deserialize files
  for(const auto& fileNode : json["files"].GetArray())
  {
    if (!fileNode.HasMember("file") || !fileNode["file"].IsString()) { LOG(LogError) << "[GuiThemeManager] Missing file in files array entry from: " << baseUrl; continue; }
    String fileName(baseUrl); fileName.Append('/').Append(fileNode["file"].GetString());
    ThemeData::Compatibility compatibility = ThemeData::ExtractCompatibility(String(fileNode["compatibility"].GetString()));
    ThemeData::Resolutions resolutions = ThemeData::ExtractResolutions(String(fileNode["resolutions"].GetString()));
    if (isEmpty(compatibility) || isEmpty(resolutions)) { LOG(LogError) << "[GuiThemeManager] Missing or wrong compatibility/resolution from: " << baseUrl << " for file " << fileName; continue; }
    ThemeDescriptor::File file({ .mFiles=fileName, .mCompatibility=compatibility, .mResolutions=resolutions, .mScreenshots={} });

    // Deserialize screenshots
    for(const auto& scrNode : fileNode["screenshots"].GetArray())
    {
      compatibility = ThemeData::ExtractCompatibility(String(scrNode["compatibility"].GetString()));
      resolutions = ThemeData::ExtractResolutions(String(scrNode["resolutions"].GetString()));
      if (isEmpty(compatibility) || isEmpty(resolutions)) { LOG(LogError) << "[GuiThemeManager] Missing or wrong compatibility/resolution in screenshot array from: " << baseUrl << " for file " << fileName; continue; }
      std::vector<String> systemFiles;   if (scrNode.HasMember("systems"))  for(const auto& screenshot : scrNode["systems"] .GetArray()) systemFiles.push_back(baseUrl + '/' + screenshot.GetString());
      std::vector<String> gameListFiles; if (scrNode.HasMember("gamelist")) for(const auto& screenshot : scrNode["gamelist"].GetArray()) gameListFiles.push_back(baseUrl + '/' + screenshot.GetString());
      std::vector<String> menuFiles;     if (scrNode.HasMember("menus"))    for(const auto& screenshot : scrNode["menus"]   .GetArray()) menuFiles.push_back(baseUrl + '/' + screenshot.GetString());
      std::vector<String> gameclipFiles; if (scrNode.HasMember("gameclip")) for(const auto& screenshot : scrNode["gameclip"].GetArray()) gameclipFiles.push_back(baseUrl + '/' + screenshot.GetString());
      bool empty = systemFiles.empty() && gameListFiles.empty() && gameclipFiles.empty() && menuFiles.empty();
      if (empty) { LOG(LogError) << "[GuiThemeManager] Empty screenshot array from: " << baseUrl << " for file " << fileName; continue; }
      file.mScreenshots.push_back( { compatibility, resolutions, systemFiles, gameListFiles, menuFiles, gameclipFiles } );
    }
    if (!file.mScreenshots.empty()) mFiles.push_back(file);
    else { LOG(LogError) << "[GuiThemeManager] No valid screenshot found from: " << baseUrl << " for file " << fileName; }
  }

  mValid = !mFiles.empty();
}
