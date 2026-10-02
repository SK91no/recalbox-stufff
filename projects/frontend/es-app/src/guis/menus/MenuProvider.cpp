//
// Created by bkg2k on 17/10/24.
//

/*
 * Disable switch all enum values warning as in this source code
 * we have big enums in several methods with only a few values really used
 * Code consistency is enforced at runtime instead
 */
#pragma GCC diagnostic ignored "-Wswitch-enum"

#include <MainRunner.h>
#include <RootFolders.h>
#include <Upgrade.h>
#include <algorithm>
#include <audio/AudioController.h>
#include <guis/menus/MenuProvider.h>
#include <guis/menus/base/ItemAction.h>
#include <guis/menus/base/ItemEditable.h>
#include <guis/menus/base/ItemRating.h>
#include <guis/menus/base/ItemSelector.h>
#include <guis/menus/base/ItemSlider.h>
#include <guis/menus/base/ItemSubMenu.h>
#include <guis/menus/base/ItemSwitch.h>
#include <hardware/devices/storage/StorageDevices.h>
#include <scraping/ScraperSeamless.h>
#include <systems/SystemManager.h>
#include "EmulationStation.h"
#include "LibretroRatio.h"
#include "MenuPads.h"
#include "MenuSearchLicenceSelector.h"
#include "MenuTate.h"
#include "MenuTools.h"
#include "games/GameFilesUtils.h"
#include "audio/AudioScheduler.h"
#include "guis/GuiBiosScan.h"
#include "guis/GuiJukebox.h"
#include "guis/GuiInformation.h"
#include "guis/GuiDownloader.h"
#include "guis/GuiInstallRecalbox.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiNetPlay.h"
#include "guis/GuiSaveStates.h"
#include "guis/GuiScraperRun.h"
#include "guis/GuiSearch.h"
#include "guis/GuiThemeManager.h"
#include "guis/menus/modaltasks/MenuModalEmulatorsReset.h"
#include "guis/menus/modaltasks/MenuModalFactoryReset.h"
#include "guis/menus/modaltasks/MenuModalHashRom.h"
#include "guis/menus/modaltasks/MenuModalInitDevice.h"
#include "guis/menus/modaltasks/MenuModalRefreshGamelist.h"
#include "guis/menus/modaltasks/MenuModalRescanRoms.h"
#include "guis/menus/modaltasks/MenuModalScanSoundBt.h"
#include "guis/menus/modaltasks/MenuModalTaskWifi.h"
#include "guis/menus/modaltasks/MenuModalTaskWps.h"
#include "guis/menus/modaltasks/MenuModalUpdateCheck.h"
#include "guis/menus/modaltasks/MenuModalRescanRoms.h"
#include "guis/GuiThemeManager.h"
#include <systems/SystemManager.h>
#include <guis/menus/base/ItemAction.h>
#include <guis/menus/base/ItemSwitch.h>
#include <guis/menus/base/ItemSlider.h>
#include <guis/menus/base/ItemSelector.h>
#include <guis/menus/base/ItemEditable.h>
#include <guis/menus/base/ItemRating.h>
#include <guis/menus/base/ItemSubMenu.h>
#include <audio/AudioController.h>
#include <scraping/ScraperSeamless.h>

#include "guis/GuiGameManual.h"
#include "guis/menus/modaltasks/MenuModalUserScript.h"
#include "hardware/RPiEepromUpdater.h"
#include "hardware/RGPProxy.h"
#include "hardware/JvsCab.h"
#include "netplay/DefaultPasswords.h"
#include "systems/arcade/ArcadeVirtualSystems.h"
#include "utils/SectionFile.h"
#include "utils/cplusplus/StaticLifeCycleControler.h"

void MenuProvider::BuildMenu(MenuContainerType identifier, const InheritableContext& context, const ItemBase* itemParent)
{
  const MenuDefinition* menu = mDeserializer.MenuDefinitions().try_get(identifier);
  if (menu == nullptr) { LOG(LogError) << "[MenuProvider] Unknown menu identifier '" << (int)identifier << '\''; return; }
  BuildMenu(*menu, context, itemParent);
}

void MenuProvider::BuildMenu(const MenuDefinition& definition, const InheritableContext& context, const ItemBase* itemParent)
{
  // Build menu GUI
  MenuBuilder* gui = new MenuBuilder(mWindow, context, *this, mDataProvider, mEventProcessor, definition, mResolver, itemParent);
  mWindow.pushGui(gui);
}

/*
 * ================================================================================================================
 * SUB MENU HANDLERS
 * ================================================================================================================
 */

void MenuProvider::SubMenuSelected(const ItemSubMenu& item, int id)
{
  MenuContainerType menuType = (MenuContainerType)id;

  // ------- TEMP CODE BEGIN
  if (menuType == MenuContainerType::Controllers) { mWindow.pushGui(new MenuPads(mWindow)); return; }
  if (menuType == MenuContainerType::Tate       ) { mWindow.pushGui(new MenuTate(mWindow, mSystemManager)); return; }
  // ------- TEMP CODE ENDS

  BuildMenu(menuType, item.Context(), &item);
}

bool MenuProvider::IsMenuUnselectable(MenuContainerType identifier)
{

  (void)identifier;
  return false;
}

/*
 * ================================================================================================================
 * TOOLS
 * ================================================================================================================
 */

String MenuProvider::SystemNameWithEmulator(const SystemData* system)
{
  String emulator;
  String core;
  String name(system->FullName());
  if (!Renderer::Instance().Is480pOrLower())
  {
    name.UpperCase();
    EmulatorManager::GetSystemEmulator(*system, emulator, core);
    if (!emulator.empty())
    {
      name.Append(" - ").Append(emulator);
      if (emulator != core) name.Append(' ').Append(core);
    }
  }
  return name;
}

bool MenuProvider::TrySortNumerically(SelectorEntry<String>::List& list)
{
  HashMap<String, int> nameToNumeric;
  for(const SelectorEntry<String>& item : list)
  {
    if (item.mText.empty()) return false;
    size_t pos = item.mText.find_first_not_of("0123456789");
    if (pos == 0) return false;
    nameToNumeric[item.mText] = (pos == std::string::npos) ? item.mText.AsInt() : item.mText.AsInt(item.mText[pos]);
  }

  std::sort(list.begin(), list.end(), [&nameToNumeric] (const SelectorEntry<String>& a, const SelectorEntry<String>& b) -> bool { return nameToNumeric[a.mText] < nameToNumeric[b.mText]; });
  return true;
}

void MenuProvider::ChangeThemeOptionSelector(ItemSelector<String>& item, const String& selected, const String::List& items)
{
  bool found = false;
  String realSelected;
  for(const String& s : items) if (s == selected) { found = true; realSelected = s; break; }
  if (!found && !items.empty()) realSelected = items.front();

  // Build list
  SelectorEntry<String>::List list;
  for (const String& s : items) list.push_back({ s, s, s == realSelected });
  // Try numeric sorting, else  use an alphanumeric sort
  if (!TrySortNumerically(list))
    std::sort(list.begin(), list.end(), [] (const SelectorEntry<String>& a, const SelectorEntry<String>& b) -> bool { return a.mText < b.mText; });

  if (!items.empty())
    item.ChangeSelectionItems(list, realSelected, list.front().mValue).SetSelectable(true);
  else
    item.ChangeSelectionItems({{ _("OPTION NOT AVAILABLE"), "" }}, "", "").SetSelectable(false);
}

void MenuProvider::CheckHighLevelFilter(bool& value, SetBoolMethod setter, const char* message, bool inverted)
{
  (mConf.*setter)(inverted ? !value : value);
  if (mSystemManager.UpdatedTopLevelFilter()) mConf.Save();
  else
  {
    mWindow.displayMessage(message == nullptr ? _("There is no game to show after this filter is changed! No change recorded.") : _(message));
    value = !value;
    (mConf.*setter)(inverted ? !value : value);
  }
}

void MenuProvider::DoJammaReset(MenuBuilder& menu)
{
  // recalbox.conf
  mConf.ResetWithFallback();
  // Set jamma config to default
  mConf.SetGlobalRewind(false);
  mConf.SetGlobalSmooth(false);
  mConf.SetQuickSystemSelect(false);
  mConf.SetAutoPairOnBoot(false);
  mConf.SetGlobalHidePreinstalled(true);
  mConf.SetAutoPairOnBoot(false);
  mConf.SetShowGameClipClippingItem(false);
  mConf.SetShowGameClipHelpItems(false);
  mConf.SetGlobalDemoInfoScreen(0);

  mConf.SetThemeFolder("recalbox-next");
  mConf.SetThemeOption("recalbox-next", "colorTheme", "3 - Older blue");
  mConf.SetThemeOption("recalbox-next", "gameclipview", "0 - DEFAULT");
  mConf.SetThemeOption("recalbox-next", "gameInfos", "1 - Show games informations");
  mConf.SetThemeOption("recalbox-next", "systemView", "1 - Horizontal");
  mConf.SetThemeOption("recalbox-next", "iconesetTheme", "8 - Jamma");
  mConf.SetThemeOption("recalbox-next", "gameList", "1 - Standard");
  mConf.SetThemeOption("recalbox-next", "shadow", "1 - No Shadow");
  mConf.SetThemeOption("recalbox-next", "SysInfos", "1 - Show light informations");
  mConf.SetThemeOption("recalbox-next", "bands", "1 - Slim");

  std::vector<String> manufacturers;
  for(const String& rawIdentifier : ArcadeVirtualSystems::GetVirtualArcadeSystemList())
  {
    String identifier(SystemManager::sArcadeManufacturerPrefix);
    identifier.Append(rawIdentifier).Replace('\\', '-');
    manufacturers.push_back(identifier);
  }
  mConf.SetCollectionArcadeManufacturers(manufacturers);
  mConf.SetGlobalHidePreinstalled(true);
  mConf.SetKodiEnabled(false);
  mConf.SetSplashEnabled(false);
  mConf.Save();

  // recalbox-crt-options.cfg
  CrtConf::Instance().ResetWithFallback();
  CrtConf::Instance().SetSystemCRT(CrtAdapterType::RGBJamma);
  CrtConf::Instance().Save();
  // REBOOT
  menu.RequestReboot();
}

void MenuProvider::RunScreenCalibration()
{
  if (Renderer::Instance().IsRotatedSide())
  {
    mWindow.pushGui(new GuiMsgBox(mWindow, _("Screen calibration only available in YOKO mode."), _("Ok"), [] {}));
    return;
  }
  if (Board::Instance().CrtBoard().GetHorizontalFrequency() == ICrtInterface::HorizontalFrequency::KHz31)
  {
    if (Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma)
    {
      ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz31_no_120);
      mWindow.CloseAll();
    }
    else
    {
      ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz31);
      mWindow.CloseAll();
    }
  }
  else if (Board::Instance().CrtBoard().MustForce50Hz())
  {
    ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_50Hz);
    mWindow.CloseAll();
  }
    /*else if (Board::Instance().CrtBoard().MultiSyncEnabled())
    {
      ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_60Hz_plus_kHz31);
      mWindow.CloseAll();
    }*/
  else if (Board::Instance().CrtBoard().GetCrtAdapter() == CrtAdapterType::RGBJamma)
  {
    ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_60Hz);
    mWindow.CloseAll();
  }
  else
  {
    mWindow.pushGui(new GuiMsgBox(mWindow, _("You will now calibrate different resolutions for the frontend. Select the refresh rate according to what your TV supports.\nDuring the calibration, press B to validate, and A to cancel."),
                                  ("60Hz & 50Hz"), [this] { ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_60plus50Hz); mWindow.CloseAll(); },
                                  _("60Hz Only"), [this] { ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_60Hz); mWindow.CloseAll(); },
                                  _("50Hz Only"), [this] { ViewController::Instance().goToCrtView(CrtCalibrationView::CalibrationType::kHz15_50Hz); mWindow.CloseAll();},
                                  Alignment::Center));
  }
}

const ScriptDescriptorList& MenuProvider::LoadUserScripts()
{
  mUserScripts = NotificationManager::Instance().GetManualScriptList();
  std::sort(mUserScripts.begin(), mUserScripts.end(), [](const ScriptDescriptor& a, ScriptDescriptor& b){ return a.mPath.ToString() < b.mPath.ToString(); });
  return mUserScripts;
}

void MenuProvider::ScrapingComplete(FileData& game, MetadataType changedMetadata, Menu* menu)
{
  (void)game;
  (void)changedMetadata;
  ((MenuBuilder*)menu)->RequestMenuRefresh();
}

/*
 * ================================================================================================================
 * SETTINGS EVENT PROCESSING
 * ================================================================================================================
 */

void MenuProvider::SettingChanged(const ItemDefinition::SettingDefinition& setting, const String& value)
{
  // Make final value
  String finalValue(value);
  switch(setting.mType)
  {
    case ItemDefinition::SettingType::List:
    case ItemDefinition::SettingType::NumericBoolean: break;
    case ItemDefinition::SettingType::Range:
    {
      String::List limits = setting.mValues.Split(':');
      if (limits.size() > 3)
        if (limits[3].StartsWith('+')) { limits[3].TrimLeft('+'); finalValue.Append(limits[3]); }
      break;
    }
    case ItemDefinition::SettingType::TrueFalseBoolean: finalValue = value == "1" ? "true" : "false"; break;
    case ItemDefinition::SettingType::YesNoBoolean: finalValue = value == "1" ? "yes" : "no"; break;
    case ItemDefinition::SettingType::CustomBoolean:
    {
      String yes;
      String no;
      if (!setting.mValues.Extract(':', yes, no, true)) { LOG(LogWarning) << "[MenuBuilder] Abnormal values property for custom boolean:" << setting.mValues; return; }
      finalValue = value == "1" ? yes : no; break;
    }
    case ItemDefinition::SettingType::Unknown:
    default: { LOG(LogFatal) << "[MenuBuilder] Abnormal ItemDefinition::SettingType::Unknown type or unprocessed type found."; break; }
  }
  // Apply properties
  if ((setting.mProps & ItemDefinition::SettingProps::Lowercase) != 0) finalValue.LowerCase();
  if ((setting.mProps & ItemDefinition::SettingProps::Uppercase) != 0) finalValue.UpperCase();
  if ((setting.mProps & ItemDefinition::SettingProps::Singlequoted) != 0) finalValue.Insert(0, '\'').Append('\'');
  if ((setting.mProps & ItemDefinition::SettingProps::DoubleQuoted) != 0) finalValue.Insert(0, '"').Append('"');
  // Try to write value
  switch(setting.mFileType)
  {
    case ItemDefinition::FileType::SectionIni:
    {
      SectionFile f(setting.mTargetFile, true);
      String section;
      String key;
      if (setting.mKey.Extract('/', section, key, true))
      {
        f.SetAsString(section, key, finalValue).Save();
        { LOG(LogDebug) << "[MenuProvider] Written emulator setting key/value [" << section << "]" << key << '/' << finalValue << " into file " << setting.mTargetFile; }
      }
      else
      {
        f.SetAsString(setting.mKey, finalValue).Save();
        { LOG(LogDebug) << "[MenuProvider] Written emulator setting key/value " << setting.mKey << '/' << finalValue << " into file " << setting.mTargetFile; }
      }

      break;
    }
    case ItemDefinition::FileType::SimpleIni:
    {
      IniFile f(setting.mTargetFile, "Writing setting", true, false);
      f.SetString(setting.mKey, finalValue).Save();
      { LOG(LogDebug) << "[MenuProvider] Written emulator setting key/value " << setting.mKey << '/' << finalValue << " into file " << setting.mTargetFile; }
      break;
    }
    case ItemDefinition::FileType::Unknown:
    default: { LOG(LogFatal) << "[MenuBuilder] Abnormal ItemDefinition::FileType::Unknown file type or unprocessed type found."; break; }
  }
}

/*
 * ================================================================================================================
 * MENU EVENT PROCESSING
 * ================================================================================================================
 */

#define LICENSE_ORIGINAL_TEXT "Redistribution and use of the RECALBOX code or any derivative works are permitted provided that the following conditions are met:\n" \
                              "\n" \
                              "- Redistributions may not be sold without authorization, nor may they be used in a commercial product or activity.\n" \
                              "- Redistributions that are modified from the original source must include the complete source code, including the source code for all components used by a binary built from the modified sources.\n" \
                              "- Redistributions must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n" \
                              "\n" \
                              "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

static const char* LICENCE_MSG = "\317'\371\066\353;\231\354W\213\261\061\222\066:2d\302`\205\312\345\063\327u\365z\255\004\333\371\061x\217?\214ߑ\240\177\331\002J\326\346\\\251*\352B\367\025D\230\330\062\210\060oI\223\262[\263\327\a\214\204\236\263\237\221=\202\345\rN\316+\337\030\225\034z\315$r\206\020~UC\030F\267}y\276\370\020\263_\236G\215\tq\246\366x\210\377\205ɏ\351P\274\355?3\307\071\303H\314=\200\264\064\r\343AT\004G5`\333;\220\240\374h\250s\260d\356\ae\315]*\270\b\367\373\267\251V\320\003\\\337\345^\346<\342\061\224ZK\206\316%\235?f2\236\357\\\212\300Y(\303g\270#\327\067\211\200\035X\246\021\343\347\333Ud\330>@\313\027M^!YS\243\v{\272\324Z\267X\227\024\233\365N\265\357\002\200\271\233\304\317\307!\250\345#u\333-\201{\207A\257\362#y\244\036}\003N\020h\321=l\270\262Q\242D\223\000\266\001h\235\353\"\204\354\206˾\366\016\256\n.e\273\064\363\070\266\"\233\352T\225\255\067\227vk>}\326]\213\227\023d\305e\363e\255<\225\334\026\\\354\005\244\353\347\035u\331\024A\221\363I\247:\355L\247\fz\310\314-\203'(\035\235\376I\266\326\016\224̚\237\214\307.\305\346\061P\312>\335Z\320\005b\305p\177\217\001\070\022Y\002\005\242\064f\244\364@\266@\204Z\300\te\243\353v\213\367\311\312\304\371^\277\340\070h\204+\321\017\363\"\322\352Nj\241\177p7Dz|\302#\236\246\356\177\333:\257z\271\032a\327\bl\337W\365ŵ\343J\316\031D\224\362@\344;\344,\255XE\204\313?\320\071h6\217\366W\226\317\026&\337n\255!\331\066\201\350\002\016\242\t\261\372\316\032h\301-\002\207\023U\025]\022\002\352\020|\264\327\027\260^\212F\311\352y\357\373\060\234\363\233\300\204\313.\251\252\062t\311r\331\a\315\177\235\372)b\272\037d\006MC~\337#c\245\361J\245F\227>\274\037+\317\354.\376\347\261ì\375I\250\002^r\360Q\367{\365%\203\256^\210\341;\215<s=&\317]\230\320\016f\325c\242*\261/\230\321\031S\257\v\352\373\254\bp\222\026C\227\245N\354\064\364E\272\017h\222\304/\213n\b1\246\306a\213\201;\264\346\257\352\267\377\f\303\332\017\003\346\t\371.\370,A\332nU\207QB9vL`\360\tP\200\320k\231x\316o\221\066P\201\337\a\307ͩ\355\302\332z\201+\032F\340\022\366+\317\aШ{P\352\024Q\177?\031A\357e\264\360\334\070\370@\212A\217\065S\220\066\022\363\061\236ł\314d\377Kc\272\334a\317\036\335\t\231%\n\317\377\a\255\005^\000\262\331v͊3Q\375$\210\260\342r\240\325(e\353!\373\241\356>!\207\003h\242ZV7l9e\215&2\214\343g\210t\271d\256\306N\355\331\033\320ص\340\250\352\177\226\334\020Z\342\034\342\071\377}\270\335\a,\225/_(m2K\224\b\261\225\256`\346q\242N\207&J\354\316\b\216\241\214˅\336e\227\061\035E\305y\267\035\333\032\246\063n\261\362\027\263|4\004J\200t\245\223?[\367C\233(\354\023\264\351\"\025\234%\215\252\200\065F\351\062`\267\313|\243\n){\224\071;\253\371\036\240qNm\242\305d\203\313)\253\315\331\371\261\213\v\250\320\030m\361y\362\071\375FC\322\025\065\230&F*r>\r\235\036I\224\336x\212i\270\t\374\061\\\201\302\026\240ı欻\023\236-\003N\351\004\354N\273\027\266\315uQ\204\032R\002\061vB\373c\260\363\315E\376G\215T\206\060\267\357;\036\225\067\234ҋ\306L\231A{\267\317}\331\n\316p\235X\004\257\341\023\314\tF\026\331\331f\222\341;G\343\"\220\262\230p\272\310,i\350\060\370\062\375!_\205\032X\305+H:i'c\222\065D\234\200t\227|\261}\305\316I\353\307\036\274ŧ\363\254\363\a\366\312\000E\372n\346-\204\016\244\324m*\225%]%\n\177Y\340l\254\227\303Y\215K\262!\361\"U\227\302\022\211\066\224ن\337\b\213<oG\307h\300\a\304\016\254\063\004\326\365\037\242\025D\016P\276{\251\344+N\364+\214H\371y\274\355Lr\210\062\346܉=[\374\a\a\241\307\006\315\n=q\206%P\266\360\020\301k\266r\272\302a\206\275-\271\274\264\366\274\375\027\236\314\f3\222\004\346&\344'T\272\006Z\233/[!f<F\227uJ\234\265|\202z\276\003\362?^\203\305\005\247\301\255\342\376\333t\214\067\rL\333\r\335%\271\037\276\246y\253\216\034\254\016R\aN\216a\260\350\310G\360Z\221/\217,\334\355;e\357#\216\310\352\316H\343G\177\261\252{\302\t\245i\213\067\002\244\364e\276\rN\023\327\300b\233\362:A\367E\356\333\367\030\255\326Aa\375@\372\071\340$Z\346\027<\254\060;\"p4\b\224\021]\204\327s\237}\272x\272_V\217\262\n\271Ҥ\216\243\337w\225\303\b:";
static const int LICENCE_MSG_SIZE = (int)sizeof(LICENSE_ORIGINAL_TEXT) - 1;
static const char MESSAGE_DECORATOR[] = "\x9D\x31\x7B\x2C\x54\xFA\x85\x0E\xAD\x65\x1B";

static String GetText()
{
  String buffer(LICENCE_MSG, LICENCE_MSG_SIZE);
  int l = (int)strlen(MESSAGE_DECORATOR);
  for (size_t i = 0; i < buffer.size(); ++i)
    buffer[i] = (char)(buffer[i] ^ (MESSAGE_DECORATOR[i % l] + (i*17)));
  return buffer;
}

void MenuProvider::MenuEditableChanged(ItemEditable& item, int id, const String& newText)
{
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionUsername: ScraperFactory::SetLogin(mConf.GetScraperSource(), newText);; break;
    case MenuItemType::ScrapeOptionPassword: ScraperFactory::SetPassword(mConf.GetScraperSource(), newText); break;
    case MenuItemType::RetroachievementsUsername: mConf.SetRetroAchievementLogin(newText).Save(); break;
    case MenuItemType::RetroachievementsPassword: mConf.SetRetroAchievementPassword(newText).Save(); break;
    case MenuItemType::NetplayNickname: mConf.SetNetplayLogin(newText).Save(); break;
    case MenuItemType::NetplayPort: { int p = newText.AsInt(0, RecalboxConf::sNetplayDefaultPort); mConf.SetNetplayPort(p).Save(); item.SetText(String(p), false); break; }
    case MenuItemType::NetworkHostname: mConf.SetHostname(newText).Save(); mWindow.displayMessage(_("Hostname changes will not be effective until next reboot")); break;
    case MenuItemType::NetworkSSID:
    case MenuItemType::NetworkPassword:
    {
      if ((MenuItemType)id == MenuItemType::NetworkSSID) mConf.SetWifiSSID(newText);
      else mConf.SetWifiKey(newText);
      mConf.Save();
      if (!mConf.GetWifiSSID().empty() && !mConf.GetWifiKey().empty())
        MenuModalTaskWIFI::CreateWIFITask(mWindow, mConf.GetWifiEnabled());
      break;
    }
    case MenuItemType::MetadataName: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetName(newText); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Name, false); break; }
    case MenuItemType::MetadataDescription: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetDescription(newText); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Synopsis, false); break; }
    case MenuItemType::NetplayPassword:
    {
      if (!newText.empty()) mConf.SetNetplayPasswords(id, newText).Save();
      else { mWindow.displayMessage(_("Passwords cannot be empty ! Replaced with default value.")); item.SetText(DefaultPasswords::sDefaultPassword[item.Context().Index()], false); }
      break;
    }
    default:
     mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuEditableChanged !");
  }
}

void MenuProvider::MenuActionTriggered(ItemAction& item, int id)
{
  (void)item;
  switch((MenuItemType)id)
  {
    case MenuItemType::Shutdown: mWindow.pushGui(new GuiMsgBox(mWindow, _("REALLY SHUTDOWN?"), _("YES"), []{ MainRunner::RequestQuit(MainRunner::ExitState::Shutdown); }, _("NO"), nullptr)); break;
    case MenuItemType::FastShutdown: mWindow.pushGui(new GuiMsgBox(mWindow, _("REALLY SHUTDOWN WITHOUT SAVING METADATAS?"), _("YES"), []{ MainRunner::RequestQuit(MainRunner::ExitState::FastShutdown); }, _("NO"), nullptr)); break;
    case MenuItemType::Restart: mWindow.pushGui(new GuiMsgBox(mWindow, _("REALLY RESTART?"), _("YES"), []{ MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot); }, _("NO"), nullptr)); break;
    case MenuItemType::RunKodi: if (!GameRunner::Instance().RunKodi()) { LOG(LogWarning) << "[Kodi] Kodi terminated with non-zero result!"; } break;
    case MenuItemType::OpenLobby: mWindow.pushGui(new GuiNetPlay(mWindow, mSystemManager)); /*Close();*/ break;
    case MenuItemType::OpenJukebox: mWindow.pushGui(new GuiJukebox(mWindow, AudioScheduler::Instance())); break;
    case MenuItemType::CheckBios: mWindow.pushGui(new GuiBiosScan(mWindow, mSystemManager)); break;
    case MenuItemType::ShowLicense: mWindow.pushGui(new GuiMsgBoxScroll(mWindow, "RECALBOX", GetText(), _("OK"), nullptr, "", nullptr, "", nullptr, Alignment::CenterLeft)); break;
    case MenuItemType::SystemInformation: mWindow.pushGui(new GuiInformation(mWindow)); break;
    case MenuItemType::ScrapeRun:
    {
      if (!ScraperFactory::Instance().HasSystems())
        mWindow.pushGui(new GuiMsgBox(mWindow, _("Please select one or more systems to scrape!"), _("OK"), nullptr));
      else
        GuiScraperRun::CreateOrShow(mWindow, mSystemManager, ScraperFactory::Instance().SystemList(), ScraperFactory::Instance().ScrapingMethod(), &GameRunner::Instance(),
                                    Renderer::Instance().Is480pOrLower());
      break;
    }
    case MenuItemType::HashRoms: MenuModalHashRom::CreateRomHasher(mWindow, mSystemManager); break;
    case MenuItemType::Changelog: { mWindow.displayScrollMessage(_("RELEASE NOTES"), Files::LoadFile(Path("/recalbox/recalbox.releasenotes"))); break; }
    case MenuItemType::UpdateChangelog:
    {
      String changelog = Upgrade::Instance().NewReleaseNote();
      if (!changelog.empty())
        mWindow.displayScrollMessage(_("AN UPDATE IS AVAILABLE FOR YOUR RECALBOX"), _("NEW VERSION:") + ' ' + Upgrade::Instance().NewVersion() + "\n" + _("UPDATE CHANGELOG:") + "\n" + changelog);
      else
        mWindow.displayMessage(_("AN UPDATE IS AVAILABLE FOR YOUR RECALBOX"));
      break;
    }
    case MenuItemType::UpdateStart: mWindow.pushGui(new GuiUpdateRecalbox(mWindow, Upgrade::Instance().TarUrl(), Upgrade::Instance().ImageUrl(), Upgrade::Instance().HashUrl(), Upgrade::Instance().NewVersion())); break;
    case MenuItemType::UpdateCheckNow: MenuModalUpdateCheck::InitiateManualUpdateCheck(mWindow, (MenuBuilder&)item.Parent()); break;
    case MenuItemType::NetworkWPS: MenuModalTaskWPS::CreateWPS(mWindow, CoItemAsEditor(item, MenuItemType::NetworkSSID), CoItemAsEditor(item, MenuItemType::NetworkPassword), CoItemAsSwitch(item, MenuItemType::NetworkConnect)); break;
    case MenuItemType::UpdateGamelists:
    {
      if (item.Context().HasSystem())
        mWindow.pushGui(new GuiMsgBox(mWindow, (_F(_("REALLY UPDATE {0}'S GAME LIST ?")) / item.Context().System()->FullName()).ToString(), _("YES"), [this, &item] {
          MenuModalRescanRoms::CreateRescanRomsTask(mWindow, mSystemManager, {item.Context().System()}); }, _("NO"), nullptr));
      else
        mWindow.pushGui(new GuiMsgBox(mWindow, _("REALLY UPDATE ALL GAME LISTS ?\n\nIT MAY TAKE A LONG TIME DEPENDING OF YOUR STORAGE SPEED AND THE NUMBER OF GAMES."), _("YES"), [this] {
          MenuModalRescanRoms::CreateRescanRomsTask(mWindow, mSystemManager, mSystemManager.AllSystems()); }, _("NO"), nullptr));
      break;
    }
    case MenuItemType::DeviceInitialize: assert(item.Context().HasDevice() && "No device context"); MenuModalInitDevice::CreateDeviceInitializer(mWindow, item.Context().Device()->ToDeviceMount(), USBInitializationAction::OnlyRomFolders); break;
    case MenuItemType::FactoryReset: MenuModalFactoryReset::InitiateFactoryReset(&mWindow); break;
    case MenuItemType::EmulatorsReset: MenuModalEmulatorsReset::InitiateEmulatorsReset(&mWindow); break;
    case MenuItemType::BootloaderUpdate:
    {
      RPiEepromUpdater updater;
      if(updater.Error()) { mWindow.pushGui(new GuiMsgBox(mWindow, _("Could not get bootloader status. Something went wrong"), _("OK"), nullptr)); return; }
      if(updater.IsUpdateAvailable())
      {
        mWindow.pushGui(new GuiMsgBox(mWindow, _("An update is available :\n").Append(_("Current version: \n")).Append(updater.CurrentVersion()).Append(_("\nLatest version: \n")).Append(updater.LastVersion()),
                                      _("UPDATE"), [this, updater]{
            { LOG(LogInfo) << "[EepromUpdate] Processing UPDATE to " << updater.LastVersion(); }
            if(updater.Update())
            {
              mWindow.pushGui(new GuiMsgBox(mWindow, _("Update success. The bootloader is up to date."), _("OK"), [&]{ MainRunner::RequestQuit(MainRunner::ExitState::NormalReboot); }));
              { LOG(LogInfo) << "[EepromUpdate] Update success to " << updater.LastVersion(); }
            }
            else
            {
              mWindow.pushGui(new GuiMsgBox(mWindow, _("Could not update bootloader. Something went wrong"), _("OK"), nullptr));
              { LOG(LogError) << "[EepromUpdate] Unable to update to " << updater.LastVersion(); }
            }
          }, _("CANCEL"), nullptr));
      }
      else mWindow.pushGui(new GuiMsgBox(mWindow, _("Your bootloader is up to date.\nThe bootloader version is:\n").Append(updater.CurrentVersion()), _("OK"), nullptr));
      break;
    }
    case MenuItemType::JammaReset: { mWindow.pushGui(new GuiMsgBox(mWindow, _("Are you sure you want to reset JAMMA configuration?"), _("YES"), [this, &item] { DoJammaReset((MenuBuilder&)item.Parent()); }, _("NO"), nullptr)); break; }
    case MenuItemType::CrtScreenCalibration: { RunScreenCalibration(); break; }
    case MenuItemType::SoundBTPairing: { MenuModalScanSoundBT::CreateScanSoundBT(mWindow); break; }
    case MenuItemType::SearchInSystem: { assert(item.Context().HasSystem() && "No system context"); mWindow.pushGui(new GuiSearch(mWindow, mSystemManager, item.Context().System())); break; }
    case MenuItemType::SearchOtherVersions:
    {
      assert(item.Context().HasSystem() && "No system context");

      String alias = item.Context().Game()->Metadata().MainAliasOrName();
      SearchForcedOptions forcedOptions = SearchForcedOptions(alias, FolderData::FastSearchContext::Name, true);
      mWindow.pushGui(new GuiSearch(mWindow, mSystemManager, item.Context().System(),  &forcedOptions));
      break;
    }
    case MenuItemType::SearchByLicence:
    {
     // ISimpleGameListView* gamelist = ViewController::Instance().GetOrCreateGamelistView(item.Context().System(), false);
      std::vector <String> licences = item.Context().Game()->Metadata().Licences().Split(" | ");
      assert(item.Context().HasSystem() && "No system context");

      if (item.Context().Game()->Metadata().HasLicences() && licences.size() == 1)
      {

        SearchForcedOptions forcedOptions = SearchForcedOptions(item.Context().Game()->Metadata().Licences(), FolderData::FastSearchContext::Licences, true);
        mWindow.pushGui(new GuiSearch(mWindow, mSystemManager, item.Context().System(), &forcedOptions));
      }
      else if (licences.size() > 1)
      {
      mWindow.pushGui(new MenuSearchLicenceSelector(mWindow, mSystemManager, *item.Context().Game(), licences));
      }
      break;
    }
    case MenuItemType::RunSaveStates: { assert(item.Context().HasGame() && "No game context"); mWindow.pushGui(new GuiSaveStates(mWindow, mSystemManager, *item.Context().Game(), nullptr, true)); break; }
    case MenuItemType::ShowManual: { assert(item.Context().HasGame() && "No game context"); mWindow.pushGui(new GuiGameManual(mWindow, *item.Context().Game())); break; }
    case MenuItemType::DeleteGame:
    {
      assert(item.Context().HasSystem() && "No system context");
      if (item.Context().System()->IsScreenshots())
        mWindow.pushGui(new GuiMsgBox(mWindow, _("DELETE SCREENSHOT, CONFIRM?"), _("YES"), [this, &item]
        {
          assert(item.Context().HasGame() && "No game context");
          (void)item.Context().Game()->RomPath().Delete();
          RootFolderData::DeleteChild(item.Context().Game());
          mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::None, true);
          mWindow.CloseAll();
        }, _("NO"), {}));
      else
      {
        assert(item.Context().HasGame() && "No game context");
        if (ScraperSeamless::Instance().IsScraping(item.Context().Game())) mWindow.pushGui(new GuiMsgBox(mWindow, _("This game is currently updating its metadata. Retry in a few seconds."), _("OK"), nullptr ));
        else MenuProvider::ShowMenu(MenuContainerType::DeleteOptions, item.Context());
      }
      break;
    }
    case MenuItemType::MetadataScrape: { assert(item.Context().HasGame() && "No game context"); mWindow.pushGui(new GuiScraperSingleGameRun<Menu*>(mWindow, mSystemManager, *item.Context().Game(), &item.Parent(), this)); break;}
    case MenuItemType::DeleteGo:
    {
      // Build lists
      HashSet<Path> list;
      HashSet<Path> mediaList;
      assert(item.Context().HasGame() && "No game context");
      if (CoItemExists(item, MenuItemType::DeleteSelectGameFiles)) for(const Path& path : CoItemAsMulti<Path>(item, MenuItemType::DeleteSelectGameFiles).AllSelectedValues()) list.insert(path);
      else { list.insert(item.Context().Game()->RomPath());        for(const Path& path : GameFilesUtils::GetGameSubFiles(*item.Context().Game())) list.insert(path); }
      if (CoItemExists(item, MenuItemType::DeleteSelectGameMedia)) for(const Path& path : CoItemAsMulti<Path>(item, MenuItemType::DeleteSelectGameMedia).AllSelectedValues()) list.insert(path);
      else                                                         for(const Path& path : GameFilesUtils::GetMediaFiles(*item.Context().Game())) list.insert(path);
      if (CoItemExists(item, MenuItemType::DeleteSelectFileExtra)) for(const Path& path : CoItemAsMulti<Path>(item, MenuItemType::DeleteSelectFileExtra).AllSelectedValues()) list.insert(path);
      else                                                         for(const Path& path : GameFilesUtils::GetGameExtraFiles(*item.Context().Game())) list.insert(path);
      if (CoItemExists(item, MenuItemType::DeleteSelectFileSaves)) for(const Path& path : CoItemAsMulti<Path>(item, MenuItemType::DeleteSelectFileSaves).AllSelectedValues()) list.insert(path);
      else                                                         for(const Path& path : GameFilesUtils::GetGameSaveFiles(*item.Context().Game())) list.insert(path);
      // No file selected?
      if (list.empty() && mediaList.empty()) mWindow.pushGui((new GuiMsgBox(mWindow, _("No file selected,\nyou must at least choose one."), _("BACK"), Alignment::CenterLeft)));
      else
      {
        // Build message
        String message = _("Game").Append(": ").Append(item.Context().Game()->Name()).Append('\n');
        message.Append(_("You are about to delete this files, confirm ?"));
        message.Append("\n\n");
        for (const auto& path: list) message.Append(Path(path).Filename()).Append('\n');
        for (const auto& path: mediaList) message.Append(Path(path).Filename()).Append('\n');
        // Delete?
        mWindow.pushGui((new GuiMsgBoxScroll(mWindow, _("DELETE ALL FILES, CONFIRM?"), message,
                                             _("YES"), [&item, list, mediaList]{ GameFilesUtils::DeleteSelectedFiles(*item.Context().Game(), list, mediaList); item.Parent().Close(); },
                                             _("NO"), nullptr, "", nullptr, Alignment::CenterLeft))->SetDefaultButton(1));
      }
      break;
    }
    case MenuItemType::DeleteCancel: { item.Parent().Close(); break; }
    case MenuItemType::SoftpatchingRunOriginal:
    {
      assert(item.Context().HasGame() && "No game context");
      assert(item.Context().HasSoftpatching() && "No softpatching context");
      item.Parent().Close();
      item.Context().Game()->Metadata().SetLastPatch(Path("original"));
      if (ISoftPatchingNotifier* notifier = item.Context().Softpatching(); notifier != nullptr) notifier->SoftPathingDisabled();
      break;
    }
    case MenuItemType::SoftpatchingRunPatched:
    {
      assert(item.Context().HasSoftpatching() && "No softpatching context");
      item.Parent().Close();
      if (ISoftPatchingNotifier* notifier = item.Context().Softpatching(); notifier != nullptr)
        notifier->SoftPatchingSelected(CoItemAsList<Path>(item, MenuItemType::SoftpatchingSelectPatch).SelectedValue());
      break;
    }
    case MenuItemType::NetplayStart:
    {
      assert(item.Context().HasGame() && "No game context");
      GameLinkedData data(mConf.GetNetplayPort(),
                          mConf.GetNetplayPasswords(mConf.GetNetplayPasswordLastForPlayer()),
                          mConf.GetNetplayPasswords(mConf.GetNetplayPasswordLastForViewer()));
      ViewController::Instance().Launch(item.Context().Game(), data, Vector3f(), !ViewController::Instance().IsInVirtualSystem());
      break;
    }
    case MenuItemType::NetplayCancel: { item.Parent().Close(); break; }
    case MenuItemType::NetplayJoinAsPlayer:
    case MenuItemType::NetplayJoinAsViewer:
    {
      assert(item.Context().HasLobbyGame() && "No lobbygame context");
      const LobbyGame& lobbyGame = *item.Context().GameLobby();
      if (lobbyGame.mGame->IsGame())
      {
        int passwordIndex = CoItemAsList<int>(item, MenuItemType::NetplayUsePassword).SelectedValue();
        String password = passwordIndex >= 0 ? (mConf.IsDefinedNetplayPasswords(passwordIndex) ? mConf.GetNetplayPasswords(passwordIndex) : DefaultPasswords::sDefaultPassword[passwordIndex] )  : String::Empty;
        bool mitm = lobbyGame.mHostMethod == 3;
        GameLinkedData data(lobbyGame.mCoreShortName,
                            mitm ? lobbyGame.mMitmIp : lobbyGame.mIp,
                            mitm ? lobbyGame.mMitmPort : lobbyGame.mPort,
                            password, password, (MenuItemType) id == MenuItemType::NetplayJoinAsViewer);
        ViewController::Instance().Launch(lobbyGame.mGame, data, Vector3f(), !ViewController::Instance().IsInVirtualSystem());
      }
      break;
    }
    case MenuItemType::UserScript:
    {
      assert(item.Context().HasScript() && "No script context");
      MenuModalUserScript::CreateUserScriptRunner(mWindow, item.Context().Script()->mIndex);
      break;
    }
    case MenuItemType::InstallDevice:
    {
      assert(item.Context().HasPhysicalDevice() && "No physical disk context");
      const PhysicalDevices::Device& disk = *item.Context().PhysicalDevice();
      const String deviceName = disk.DisplayableName();
      const String devicePath = disk.DevicePath;

      // Second popup: durée + interdiction de couper + annulation possible (CANCEL par défaut)
      std::function<void()> showInstallConfirmation = [this, deviceName, devicePath]()
      {
        String confirmBody = _("The installation of Recalbox on **{DEVICE}** may take **several minutes**.\n\n"
                               "⚠ **Do NOT** shut down or reboot Recalbox during the installation.\n"
                               "If the process is interrupted, the disk may be left in an unusable state.\n\n"
                               "You can still change your mind and **cancel right now**.");
        confirmBody.Replace("{DEVICE}", deviceName);
        std::function<void()> startInstall = [this, devicePath, deviceName]
        {
          mWindow.pushGui(new GuiInstallRecalbox(mWindow, mSystemManager, devicePath, deviceName));
        };
        mWindow.pushGui((new GuiMsgBoxScroll(mWindow,
                                             _("LAST WARNING BEFORE INSTALLATION"),
                                             confirmBody,
                                             _("START"), startInstall,
                                             _("CANCEL"), nullptr, Alignment::TopLeft))->SetDefaultButton(1));
      };

      // First popup: avertissement destructif (NO par défaut) — sauté si le disque est vierge
      if (disk.HasDataOnPartitions)
      {
        String body = _("**{DEVICE}** contains **partitions with existing data**.\n\n"
                        "If you confirm, Recalbox will **permanently and irreversibly**:\n\n"
                        "- Erase **all partitions** on this disk\n"
                        "- **Destroy all data** present on those partitions\n\n"
                        "**There is no going back.** Once confirmed, there is absolutely "
                        "no way to recover your files or partitions.\n\n"
                        "Are you **absolutely sure** you want to continue?");
        body.Replace("{DEVICE}", deviceName);
        mWindow.pushGui((new GuiMsgBoxScroll(mWindow,
                                             _("WARNING ! DESTRUCTIVE OPERATION !"),
                                             body,
                                             _("YES"), showInstallConfirmation,
                                             _("NO"), nullptr, Alignment::TopLeft))->SetDefaultButton(1));
      }
      else if (disk.HasPartitions)
      {
        String body = _("**{DEVICE}** already has partitions.\n\n"
                        "If you confirm, Recalbox will **permanently and irreversibly "
                        "erase all existing partitions** on this disk.\n\n"
                        "**There is no going back.**\n\n"
                        "Are you sure you want to continue?");
        body.Replace("{DEVICE}", deviceName);
        mWindow.pushGui((new GuiMsgBoxScroll(mWindow,
                                             _("WARNING ! DESTRUCTIVE OPERATION !"),
                                             body,
                                             _("YES"), showInstallConfirmation,
                                             _("NO"), nullptr, Alignment::TopLeft))->SetDefaultButton(1));
      }
      else
      {
        // Disque vierge : on passe directement à la seconde popup
        showInstallConfirmation();
      }
      break;
    }
    case MenuItemType::DownloadGamePack:
    {
      assert(item.Context().HasSystem() && "No system context");
      mWindow.pushGui(new GuiDownloader(mWindow, *item.Context().System(), mSystemManager));
      break;
    }
    case MenuItemType::JumpToLetter:
    {
      mWindow.CloseAll();
      assert(item.Context().HasSystem() && "No system context");
      ISimpleGameListView* gamelist = ViewController::Instance().GetOrCreateGamelistView(item.Context().System(), false);
      gamelist->ShowQuickJump();
      break;
    }
    case MenuItemType::ThemeManager:
    {
      mWindow.pushGui(new GuiThemeManager(mWindow, mResolver));
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuActionTriggered !");
  }
}

void MenuProvider::MenuSwitchChanged(const ItemSwitch& item, bool& value, int id)
{
  switch((MenuItemType)id)
  {
    case MenuItemType::EnableKodi: mConf.SetKodiEnabled(value).Save(); break;
    case MenuItemType::KodiOnStartup: mConf.SetKodiAtStartup(value).Save(); break;
    case MenuItemType::KodiOnX: mConf.SetKodiXButton(value).Save(); break;
    case MenuItemType::ScrapeOptionManual: mConf.SetScreenScraperWantManual(value).Save(); break;
    case MenuItemType::ScrapeOptionMap: mConf.SetScreenScraperWantMaps(value).Save(); break;
    case MenuItemType::ScrapeOptionP2K: mConf.SetScreenScraperWantP2K(value).Save(); break;
    case MenuItemType::ScrapeOptionTips: mConf.SetScreenScraperWantTips(value).Save(); break;
    case MenuItemType::Rewind: mConf.SetGlobalRewind(value).Save(); break;
    case MenuItemType::ShowSaveStates: mConf.SetGlobalShowSaveStateBeforeRun(value).Save(); if (value) CoItemAsSwitch(item, MenuItemType::AutoSave).SetState(false, true); break;
    case MenuItemType::AutoSave: mConf.SetGlobalAutoSave(value).Save(); if (value) CoItemAsSwitch(item, MenuItemType::ShowSaveStates).SetState(false, true); break;
    case MenuItemType::PressTwice: mConf.SetGlobalQuitTwice(value).Save(); break;
    case MenuItemType::RecalboxOverlays: mConf.SetGlobalRecalboxOverlays(value).Save(); break;
    case MenuItemType::Smooth: mConf.SetGlobalSmooth(value).Save(); break;
    case MenuItemType::IntegerScale: mConf.SetGlobalIntegerScale(value).Save(); break;
    case MenuItemType::HDMode:
    {
      if (value) mWindow.displayMessage(_("RECALBOX WILL AUTOMATICALLY SELECT EMULATORS TO RUN ON HD-COMPATIBLE SYSTEMS. SOME GAMES MAY NOT BE 100% COMPATIBLE, SO DON'T FORGET TO DISABLE THIS OPTION IF EMULATION OR PERFORMANCE ISSUES APPEAR."));
      mConf.SetGlobalHDMode(value).Save();
      break;
    }
    case MenuItemType::WideScreen:
    {
      if (value) mWindow.displayMessage(_("RECALBOX WILL AUTOMATICALLY SELECT EMULATORS TO RUN ON WIDESCREEN-COMPATIBLE SYSTEMS. SOME GAMES MAY NOT BE 100% COMPATIBLE, SO DON'T FORGET TO DISABLE THIS OPTION IF EMULATION OR PERFORMANCE ISSUES APPEAR."));
      mConf.SetGlobalWidescreenMode(value).Save();
      break;
    }
    case MenuItemType::VulkanDriver: mConf.SetGlobalVulkanDriver(value).Save(); break;
    case MenuItemType::Retroachievements: mConf.SetRetroAchievementOnOff(value).Save(); break;
    case MenuItemType::RetroachievementsHardcore: mConf.SetRetroAchievementHardcore(value).Save(); break;
    case MenuItemType::RetroachievementsSounds: mConf.SetRetroAchievementSounds(value).Save(); break;
    case MenuItemType::Netplay: mConf.SetNetplayEnabled(value).Save(); break;
    case MenuItemType::UpdateCheck: mConf.SetUpdatesEnabled(value).Save(); break;
    case MenuItemType::NetworkEnableWIFI: mConf.SetWifiEnabled(value).Save(); MenuModalTaskWIFI::CreateWIFITask(mWindow, value); ((MenuBuilder&)item.Parent()).RequestMenuRefresh(); break;
    case MenuItemType::NetworkConnect: mConf.SetWifiConnect(value).Save(); MenuModalTaskWIFI::CreateWIFITask(mWindow, value); ((MenuBuilder&)item.Parent()).RequestMenuRefresh(); break;
    case MenuItemType::ShowOnlyLastVersions: CheckHighLevelFilter(value, &RecalboxConf::SetShowOnlyLatestVersion, nullptr, false); break;
    case MenuItemType::ShowOnlyFavorites: CheckHighLevelFilter(value, &RecalboxConf::SetFavoritesOnly, "There is no favorite games in any system!", false); break;
    case MenuItemType::ShowFavoriteFirst: mConf.SetFavoritesFirst(value).Save(); break;
    case MenuItemType::ShowHiddenGames: CheckHighLevelFilter(value, &RecalboxConf::SetShowHidden, nullptr, false); break;
    case MenuItemType::ShowMahjongCasino: CheckHighLevelFilter(value, &RecalboxConf::SetHideBoardGames, nullptr, true); break;
    case MenuItemType::ShowAdultGames: CheckHighLevelFilter(value, &RecalboxConf::SetFilterAdultGames, nullptr, true); break;
    case MenuItemType::ShowPreinstalledGames:  CheckHighLevelFilter(value, &RecalboxConf::SetGlobalHidePreinstalled, nullptr, true); break;
    case MenuItemType::Show3PlayerGames: CheckHighLevelFilter(value, &RecalboxConf::SetShowOnly3PlusPlayers, nullptr, false); break;
    case MenuItemType::ShowOnlyYoko: if (ItemSwitch& co = CoItemAsSwitch(item, MenuItemType::ShowOnlyTate); value && co.State()) co.SetState(false, true); CheckHighLevelFilter(value, &RecalboxConf::SetShowOnlyYokoGames, nullptr, false); break;
    case MenuItemType::ShowOnlyTate: if (ItemSwitch& co = CoItemAsSwitch(item, MenuItemType::ShowOnlyYoko); value && co.State()) co.SetState(false, true); CheckHighLevelFilter(value, &RecalboxConf::SetShowOnlyTateGames, nullptr, false); break;
    case MenuItemType::ShowNonGames: CheckHighLevelFilter(value, &RecalboxConf::SetHideNoGames, nullptr, true); break;
    case MenuItemType::ThemeCarousel: mConf.SetThemeCarousel(value).Save(); break;
    case MenuItemType::QuickSelectSystem: mConf.SetQuickSystemSelect(value).Save(); break;
    case MenuItemType::ListFastMoveEnabled: mConf.SetListFastMoveEnabled(value).Save(); break;
    case MenuItemType::OnScreenHelp: mConf.SetShowHelp(value).Save(); break;
    case MenuItemType::SwapValidateCancel: mConf.SetSwapValidateAndCancel(value).Save(); mWindow.UpdateHelpSystem(); break;
    case MenuItemType::OSDClock: mConf.SetClock(value).Save(); break;
    case MenuItemType::OneGameOneRom: mConf.SetOneGameOneRom(value).Save(); break;
    case MenuItemType::DisplayByFilename: mConf.SetDisplayByFileName(value).Save(); break;
    case MenuItemType::VirtualAllGames: mConf.SetCollectionAllGames(value).Save(); break;
    case MenuItemType::VirtualMultiplayer: mConf.SetCollectionMultiplayer(value).Save(); break;
    case MenuItemType::VirtualLastPlayed: mConf.SetCollectionLastPlayed(value).Save(); break;
    case MenuItemType::VirtualLightgun: mConf.SetCollectionLightGun(value).Save(); break;
    case MenuItemType::VirtualDial: mConf.SetCollectionDial(value).Save(); break;
    case MenuItemType::VirtualTrackball: mConf.SetCollectionTrackball(value).Save(); break;
    case MenuItemType::VirtualPorts: mConf.SetCollectionPorts(value).Save(); break;
    case MenuItemType::ArcadeEnhancedView: mConf.SetArcadeViewEnhanced(value).Save(); break;
    case MenuItemType::ArcadeFoldClones: mConf.SetArcadeViewFoldClones(value); break;
    case MenuItemType::ArcadeHideBios: mConf.SetArcadeViewHideBios(value); break;
    case MenuItemType::ArcadeHideNonWorking: mConf.SetArcadeViewHideNonWorking(value); break;
    case MenuItemType::ArcadeUseOfficielNames: mConf.SetArcadeUseDatabaseNames(value); break;
    case MenuItemType::ArcadeAggregateEnable: mConf.SetCollectionArcade(value).Save(); ((MenuBuilder&)item.Parent()).RequestMenuRefresh(); break;
    case MenuItemType::ArcadeAggregateOriginal: mConf.SetCollectionArcadeHideOriginals(value).Save(); break;
    case MenuItemType::BootOnKodi: mConf.SetKodiAtStartup(value).Save(); break;
    case MenuItemType::BootDoNotScan: mConf.SetStartupGamelistOnly(value).Save(); break;
    case MenuItemType::AllowBootOnGame:
    {
      mConf.SetBootOnGameEnabled(value).Save();
      if (value)
        mWindow.pushGui(new GuiMsgBox(mWindow, _("If no configured controller is detected at boot, Recalbox will run as usual and display the system list."), _("OK")));
      break;
    }
    case MenuItemType::BootOnGamelist: mConf.SetStartupStartOnGamelist(value).Save(); break;
    case MenuItemType::BootShowVideo: mConf.SetSplashEnabled(value).Save(); break;
    case MenuItemType::HideSystemView: mConf.SetStartupHideSystemView(value).Save(); break;
    case MenuItemType::AdvDebugLogs: MainRunner::SetDebugLogs(value, false); mConf.SetDebugLogs(value).Save(); break;
    case MenuItemType::AdvShowFPS: mConf.SetGlobalShowFPS(value).Save(); break;
    case MenuItemType::WebManagerEnable: mConf.SetSystemManagerEnabled(value); break;
    case MenuItemType::PerSystemSmooth: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemSmooth(*item.Context().System(), value).Save(); break;
    case MenuItemType::PerSystemRewind: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemRewind(*item.Context().System(), value).Save(); break;
    case MenuItemType::PerSystemAutoLoadSave: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemAutoSave(*item.Context().System(), value).Save(); break;
    case MenuItemType::Jamma4PlayerMode:
    {
      CrtConf::Instance().SetSystemCRTJamma4Players(value).Save();
      auto setStatus = [this, &value]{ CheckHighLevelFilter(value, &RecalboxConf::SetShowOnly3PlusPlayers, nullptr, false); };
      if (value && !mConf.GetShowOnly3PlusPlayers())
        mWindow.pushGui(new GuiMsgBox(mWindow, _("Do you want to enable the 3+ player filter for all the games ?"), _("NO"), [] {}, _("YES"), setStatus));
      if (!value && mConf.GetShowOnly3PlusPlayers())
        mWindow.pushGui(new GuiMsgBox(mWindow, _("Do you want to disable the 3+ player filter for all the games ?"), _("NO"), [] {}, _("YES"), setStatus));
      break;
    }
    case MenuItemType::JammaCredit: mCrtConf.SetSystemCRTJammaStartBtn1Credit(value).Save(); RGPProxy::Instance().Apply(); break;
    case MenuItemType::JammaHotkey: mCrtConf.SetSystemCRTJammaHKOnStart(value).Save(); RGPProxy::Instance().Apply(); break;
    case MenuItemType::JammaVolume: mCrtConf.SetSystemCRTJammaSoundOnStart(value).Save(); RGPProxy::Instance().Apply(); break;
    case MenuItemType::JammaExit: mCrtConf.SetSystemCRTJammaExitOnStart(value).Save(); RGPProxy::Instance().Apply(); break;
    case MenuItemType::JvsStartQuickExit: JvsCab::SetStartQuickExit(value); break;
    case MenuItemType::JvsSkipBootLogo:   JvsCab::SetSkipBootLogo(value); break;
    case MenuItemType::JvsFlipGunX:       JvsCab::SetFlipGunX(value); break;
    case MenuItemType::JvsFlipGunY:       JvsCab::SetFlipGunY(value); break;
    case MenuItemType::JammaAutoFire: mCrtConf.SetSystemCRTJammaAutoFire(value).Save(); break;
    case MenuItemType::JammaDualJoystick: mCrtConf.SetSystemCRTJammaDualJoysticks(value).Save(); break;
    case MenuItemType::JammaPinE27: mCrtConf.SetSystemCRTJammaButtonsOnJamma(value ? "5" : "6").Save(); break;
    case MenuItemType::CrtHDMIPriority: mCrtConf.SetSystemCRTForceHDMI(value).Save(); break;
    case MenuItemType::CrtWideScreen: mCrtConf.SetSystemCRTWideScreen(value).Save(); break;
    case MenuItemType::CrtSelectRefreshAtLaunch: mCrtConf.SetSystemCRTGameRegionSelect(value).Save(); break;
    case MenuItemType::CrtSelectResolutionAtLaunch: mCrtConf.SetSystemCRTGameResolutionSelect(value).Save(); break;
    case MenuItemType::CrtSelectSignalAtLaunch: mCrtConf.SetSystemCRTSignalSelect(value).Save(); break;
    case MenuItemType::CrtForceJack: mCrtConf.SetSystemCRTForceJack(value).Save(); break;
    case MenuItemType::CrtDemoAuto240: mCrtConf.SetSystemCRTRunDemoAndAutoIn240pOn31kHz(value).Save(); break;
    case MenuItemType::CrtReducedLantency: mConf.SetGlobalReduceLatency(value).Save(); break;
    case MenuItemType::CrtRunAhead: mConf.SetGlobalRunAhead(value).Save(); break;
    case MenuItemType::CrtAvoidInterlaced: mCrtConf.SetSystemCRTAvoidInterlaced(value).Save(); break;
    case MenuItemType::CrtAvoidLowFreqModes: mCrtConf.SetSystemCRTAvoidLowFreqModes(value).Save(); break;
    case MenuItemType::CrtForceComposite: mCrtConf.SetSystemCRTForceComposite(value).Save(); break;
    case MenuItemType::CrtAvoidInterlacedTateOnYokoAndHandhelds: mCrtConf.SetSystemCRTAvoidInterlacedOnTateOnYokoAndHandhelds(value).Save(); break;
    case MenuItemType::SetGameToBootOn:
    {
      assert(item.Context().HasGame() && "No game context");
      if (FileData* game = item.Context().Game(); game->IsGame())
        mConf.SetBootOnGameGamePath(value ? game->RomPath().ToString() : String::Empty)
             .SetBootOnGameSystemUUID(value ? game->System().Descriptor().GUID() : String::Empty)
             .Save();
      break;
    }
  case MenuItemType::SetGameForCard:
    {
      assert(item.Context().HasGame() && "No game context");
      if (FileData* game = item.Context().Game(); game->IsGame())
      {
        if (value)
          CardReader::Instance().SetGameForCard(game->RomPath().ToString(), game->System().Descriptor().GUID());
        else
          CardReader::Instance().DeleteGameForCard();
      }
      break;
    }
    case MenuItemType::CRConsoleMode: { mConf.SetCRConsoleMode(value).Save(); break; }
    case MenuItemType::MetadataFavorite: assert(item.Context().HasGame() && "No game context"); ViewController::Instance().ToggleFavorite(item.Context().Game(), true, value); break;
    case MenuItemType::MetadataHidden: assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetHidden(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Hidden, false); break;
    case MenuItemType::MetadataAdult: assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetAdult(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Adult, false); break;
    case MenuItemType::MetadataRotation: assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetRotation(value ? RotationType::Left : RotationType::None); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Rotation, false); break;
    case MenuItemType::EnableFavorites: mConf.SetEnableEditFavorites(value); break;
    case MenuItemType::FlattenFolders: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemFlatFolders(*item.Context().System(), value); break;
    case MenuItemType::ScrapeAuto: mConf.SetScraperAuto(value); break;
    case MenuItemType::_Settings_: assert(item.Context().HasSetting() && "No setting context"); SettingChanged(*item.Context().Setting(), value ? "1" : "0"); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSwitchChanged !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<StorageDevices::Device>& item, int id, int index, const StorageDevices::Device& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ShareDevice: StorageDevices::Instance().SetShareDevice(value); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<StorageDevices::Device> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<String>& item, int id, int index, const String& value)
{
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::Language: mConf.SetSystemLanguage(value).Save(); break;
    case MenuItemType::TimeZone: mConf.SetSystemTimezone(value).Save(); break;
    case MenuItemType::Keyboard: mConf.SetSystemKbLayout(value).Save(); break;
    case MenuItemType::KodiResolution: if (value.empty()) mConf.DeleteKodiVideoMode().Save(); else mConf.SetKodiVideoMode(value).Save(); break;
    case MenuItemType::SuperGameboyMode: mConf.SetSuperGameBoy(value).Save(); break;
    case MenuItemType::GameRatio: mConf.SetGlobalRatio(value).Save(); break;
    case MenuItemType::ShaderSet: mConf.SetGlobalShaderSet(value).Save(); break;
    case MenuItemType::AdvancedShaders:
    {
      if (value != "none" && (mConf.GetGlobalSmooth()))
        mWindow.pushGui(new GuiMsgBox(mWindow,
                                      _("YOU JUST ACTIVATED THE SHADERS FOR ALL SYSTEMS. FOR A BETTER RENDERING, IT IS ADVISED TO DISABLE GAME SMOOTHING. DO YOU WANT TO CHANGE THIS OPTION AUTOMATICALLY?"),
                                      _("LATER"), nullptr, _("YES"), [this, &item]
                                      { CoItemAsSwitch(item, MenuItemType::Smooth).SetState(false, true); }));
      mConf.SetGlobalShaders(value).Save();
      break;
    }
    case MenuItemType::AutoBlitter: mConf.SetGlobalAutoBlitter(value).Save(); break;
    case MenuItemType::NetworkPickSSID: if (!value.empty()) CoItemAsEditor(item, MenuItemType::NetworkSSID).SetText(value, true); break;
    case MenuItemType::ThemeRegion: { mConf.SetThemeRegion(value).Save(); ThemeManager::Instance().DoThemeChange(&mWindow, false); break; }
    case MenuItemType::ThemeOptions:
    {
      assert(item.Context().HasOption() && "No theme option context");
      String s = mConf.GetThemeFolder(); IniFile::PurgeKey(s);
      mConf.SetThemeOption(s, item.Context().Option()->Name(), value).Save();
      ThemeManager::Instance().DoThemeChange(&mWindow, false);
      // force hasdefaulttransition check to grey unneeded options
      ThemeManager::Instance().Main().getThemeTransition();
      ((MenuBuilder&)item.Parent()).RequestMenuRefresh();
      break;
    }
    case MenuItemType::BootOnSystem: mConf.SetStartupSelectedSystem(value).Save(); break;
    case MenuItemType::PerSystemEmulator:
    {
      assert(item.Context().HasSystem() && "No system context");
      const SystemData& system = *item.Context().System();
      // Split emulator & core
      String emulator;
      String core;
      if (value.Extract(':', emulator, core, false))
      {
        // Set emulator/core
        String defaultEmulator;
        String defaultCore;
        EmulatorManager::GetDefaultEmulator(system, defaultEmulator, defaultEmulator);
        if (emulator == defaultEmulator && core == defaultCore) mConf.SetSystemEmulator(system, "").SetSystemCore(system, "").Save();
        else mConf.SetSystemEmulator(system, emulator).SetSystemCore(system, core).Save();
        { LOG(LogError) << "[SystemConfigurationGui] Switching to " << emulator << " emulator and " << core << " core for system " << system.FullName(); }
      }
      else
      {
        mConf.DeleteSystemEmulator(system);
        { LOG(LogError) << "[SystemConfigurationGui] Switching to default emulator and core for system " << system.FullName(); }
      }
      break;
    }
    case MenuItemType::PerSystemRatio: { assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemRatio(*item.Context().System(), value).Save(); break; }
    case MenuItemType::PerSystemShaderSet: { assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemShaderSet(*item.Context().System(), value).Save(); break; }
    case MenuItemType::PerSystemShaders: { assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemShaders(*item.Context().System(), value).Save(); break; }
    case MenuItemType::PerSystemCrtHandheldFormat: { assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemCrtHandheldFormat(*item.Context().System(), value).Save(); break; }
    case MenuItemType::CrtHandheldFormat: { mConf.SetGlobalCrtHandheldFormat(value).Save(); break; }
    case MenuItemType::CrtMenuResolution: { mCrtConf.SetSystemCRTResolution(value).Save(); break; }
    case MenuItemType::CrtSuperrezMultiplier: { mCrtConf.SetSystemCRTSuperrez(value).Save(); break; }
    case MenuItemType::CrtCompositeStandard: { mCrtConf.SetSystemCRTCompositeStandard(value).Save(); break; }
    case MenuItemType::CRConsoleModeExitMode: { mConf.SetCRConsoleModeExit(value).Save(); break; }
    case MenuItemType::CRConsoleModeVideo: { mConf.SetCRConsoleModeVideo(value).Save(); break; }
    case MenuItemType::CRConsoleModeSound: { mConf.SetCRConsoleModeSound(value).Save(); break; }
    case MenuItemType::JammaSound: mCrtConf.SetSystemCRTJammaAmpDisable(value == "1").Save(); break;
    case MenuItemType::JammaAmpBoost: mCrtConf.SetSystemCRTJammaMonoAmpBoost(value).Save(); break;
    case MenuItemType::JammaPanelType: mCrtConf.SetSystemCRTJammaPanelButtons(value).Save(); break;
    case MenuItemType::JvsPlayersOnPanel: JvsCab::SetPlayersOnPanel(value.c_str()); break;
    case MenuItemType::JvsCabType:        JvsCab::SetType(value.c_str()); ((MenuBuilder&)item.Parent()).RequestMenuRefreshIn(1500); break;
    case MenuItemType::JvsLinkMode:       JvsCab::SetLinkMode(value.c_str()); break;
    case MenuItemType::JammaNeogeoLayoutP1: CrtConf::Instance().SetSystemCRTJammaNeogeoLayoutP1(value).Save(); break;
    case MenuItemType::JammaNeogeoLayoutP2: CrtConf::Instance().SetSystemCRTJammaNeogeoLayoutP2(value).Save(); break;
    case MenuItemType::Case:
    {
      Case selectedCase = Case::FromShortName(value);
      Case currentCase = Case::CurrentCase();
      const auto install = [selectedCase, &item] {
          bool installed = selectedCase.Install();
          { LOG(LogInfo) << "[Settings - Cases] Installed case " << selectedCase.DisplayName() << (installed ? " successfully" : "failed!"); }
          ((MenuBuilder&)item.Parent()).RequestReboot();
      };
      if (selectedCase.Model() != currentCase.Model())
      {
        const String installMessage = selectedCase.GetInstallMessage();
        if (!installMessage.empty())
          mWindow.pushGui(new GuiMsgBox(mWindow, installMessage, _("OK"), install));
        else {
          install();
        }
      }
      break;
    }
    case MenuItemType::ResolutionGlobal: mConf.SetGlobalVideoMode(value).Save(); break;
    case MenuItemType::ResolutionFrontEnd: mConf.SetEmulationstationVideoMode(value).Save(); break;
    case MenuItemType::ResolutionEmulators: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemVideoMode(*item.Context().System(), value).Save(); break;
    case MenuItemType::SoundAudioOutput:
    {
      AudioController::Instance().DisableNotification();
      AudioController::Instance().SetDefaultPlayback(value);
      AudioController::Instance().SetVolume(mConf.GetAudioVolume());
      AudioController::Instance().EnableNotification();
      mConf.SetAudioOuput(value).Save();
      break;
    }
    case MenuItemType::MetadataSetEmulator:
    {
      assert(item.Context().HasGame() && "No game context");
      FileData& game = *item.Context().Game();
      game.Metadata().SetEmulator(String::Empty);
      game.Metadata().SetCore(String::Empty);
      // Split emulator & core
      String emulator;
      String core;
      String defaultEmulator;
      String defaultCore;
      bool dummy;
      EmulatorManager::GetGameEmulatorOverriden(game, defaultEmulator, defaultCore, dummy);
      if (value.Extract(':', emulator, core, false))
        if (emulator != defaultEmulator || core != defaultCore)
        {
          game.Metadata().SetEmulator(emulator);
          game.Metadata().SetCore(core);
        }
      mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Emulator, false);
      break;
    }
    case MenuItemType::MetadataRatio: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetRatio(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Ratio, false); break; }
    case MenuItemType::_Settings_: assert(item.Context().HasSetting() && "No setting context"); SettingChanged(*item.Context().Setting(), value); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<String> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperImageType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperImageType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionImage: mConf.SetScreenScraperMainMedia(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScreenScraperEnums::ScreenScraperImageType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperBoxType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperBoxType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionBox: mConf.SetScreenScraperBox(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScreenScraperEnums::ScreenScraperBoxType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperLogoType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperLogoType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionLogo: mConf.SetScreenScraperLogo(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScreenScraperEnums::ScreenScraperLogoType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::Transition>& item, int id, int index, const RecalboxConf::Transition& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ThemeTransition: mConf.SetThemeTransition(value).Save(); break;
    case MenuItemType::GameTransition: mConf.SetGameTransition(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::Transition> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperVideoType>& item, int id, int index, const ScreenScraperEnums::ScreenScraperVideoType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionVideo: mConf.SetScreenScraperVideo(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScreenScraperEnums::ScreenScraperVideoType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScreenScraperEnums::ScreenScraperRegionPriority>& item, int id, int index, const ScreenScraperEnums::ScreenScraperRegionPriority& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionRegionPriority: mConf.SetScreenScraperRegionPriority(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScreenScraperRegionPriority> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<Regions::GameRegions>& item, int id, int index, const Regions::GameRegions& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionRegionFavorite: mConf.SetScreenScraperRegion(value).Save(); break;
    case MenuItemType::RomPreferredRegion: mConf.SetRomPreferredRegion(value).Save(); break;
    case MenuItemType::RomSecondPreferredRegion: mConf.SetRomSecondPreferredRegion(value).Save(); break;
    case MenuItemType::HighlightRegion: assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemRegionFilter(*item.Context().System(), value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<Regions::GameRegions> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<Regions::OrderedMasterRegions>& item, int id, int index, const Regions::OrderedMasterRegions& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::RomPreferredFallback: mConf.SetRomPreferredFallback(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<Regions::OrderedMasterRegions> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<Languages>& item, int id, int index, const Languages& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionLanguageFavorite: mConf.SetScreenScraperLanguage(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<Languages> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScraperNameOptions>& item, int id, int index, const ScraperNameOptions& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeOptionNameFrom: mConf.SetScraperNameOptions(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ScraperNameOptions> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScrapingMethod>& item, int id, int index, const ScrapingMethod& value)
{
  (void)item;
  (void)index;
  (void)value;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeFilter: ScraperFactory::Instance().SetScrapingMethod(value); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<ScrapingMethod*> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ScraperType>& item, int id, int index, const ScraperType& value)
{
  (void)item;
  (void)index;
  (void)value;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeFrom:
    {
      mConf.SetScraperSource(value).Save();
      CoItemAsEditor(item, MenuItemType::ScrapeOptionUsername).SetSelectable(ScraperFactory::HasCredentials(value));
      CoItemAsEditor(item, MenuItemType::ScrapeOptionPassword).SetSelectable(ScraperFactory::HasCredentials(value));
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<ScraperType*> !");
  }
}

void MenuProvider::MenuMultiChanged(const ItemSelectorBase<SystemData*>& item, int id, int index, const std::vector<SystemData*>& value)
{
  (void)item;
  (void)index;
  (void)value;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScrapeSystems:
    {
      for(SystemData* system : item.AllSelectedValues()) ScraperFactory::Instance().AddSystem(system);
      for(SystemData* system : item.AllUnselectedValues()) ScraperFactory::Instance().RemoveSystem(system);
      break;
    }
    case MenuItemType::ShowSystems:
    {
      if (index >= 0) mConf.SetSystemIgnore(*item.SelectedValue(), !item.SelectedState());
      else
      {
        if (!value.empty())
          for (SystemData* system: value)
            mConf.SetSystemIgnore(*system, false);
        else
        {
          for (SystemData* system: item.AllUnselectedValues())
            if (system->Manager().VisibleSystemList().Contains(system))
              mConf.SetSystemIgnore(*system, true);
        }
        mConf.Save();
      }
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<vector<SystemData*>> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::SoftPatching>& item, int id, int index, const RecalboxConf::SoftPatching& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::SoftPatching: mConf.SetGlobalSoftpatching(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::SoftPatching> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::Relay>& item, int id, int index, const RecalboxConf::Relay& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::NetplayRelay: mConf.SetNetplayRelay(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::Relay> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::VirtualKeyboardType>& item, int id, int index, const RecalboxConf::VirtualKeyboardType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::VirtualKeyboard: mConf.SetVirtualKeyboardType(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::VirtualKeyboardType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::UpdateType>& item, int id, int index, const RecalboxConf::UpdateType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::UpdateType: mConf.SetUpdateType(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::UpdateType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::GamelistLayout>& item, int id, int index, const RecalboxConf::GamelistLayout& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    // Selecting the default value removes the system key rather than writing it,
    // so the file only carries real choices (see SetSystemGamelistLayout).
    case MenuItemType::GamelistLayout: mConf.SetSystemGamelistLayout(*item.Context().System(), value).Save(); break;
    case MenuItemType::GamelistLayoutGlobal: mConf.SetGlobalGamelistLayout(value).Save(); break;
    case MenuItemType::GamelistLayoutPerSystem:
    {
      assert(item.Context().HasSystem() && "No system context");
      mConf.SetSystemGamelistLayout(*item.Context().System(), value).Save();
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::GamelistLayout> !");
  }
}

void MenuProvider::MenuSliderMoved(ItemSlider& item, int id, float value)
{
  switch((MenuItemType)id)
  {
    case MenuItemType::ScreensaverTimeout: mConf.SetScreenSaverTime((int)value).Save(); break;
    case MenuItemType::PopupHelpDuration: mConf.SetPopupHelp((int)value).Save(); break;
    case MenuItemType::PopupMusic: mConf.SetPopupMusic((int)value).Save(); break;
    case MenuItemType::PopupNetplay: mConf.SetPopupNetplay((int)value).Save(); break;
    case MenuItemType::Brightness: if (mConf.GetBrightness() != (int)value) { Board::Instance().SetBrightness((int) value); mConf.SetBrightness((int) value).Save(); } break;
    case MenuItemType::SoundSystemVolumem: { int v = Math::roundi(value); AudioController::Instance().SetVolume(v); mConf.SetAudioVolume(v).Save(); break; }
    case MenuItemType::SoundMusicVolume: if (int v = Math::roundi(value); AudioController::Instance().GetMusicVolume() !=v) { AudioController::Instance().SetMusicVolume(v); mConf.SetAudioMusicVolume(v).Save(); } break;
    case MenuItemType::_Settings_: assert(item.Context().HasSetting() && "No setting context"); SettingChanged(*item.Context().Setting(), String((int)value)); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSliderMoved !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<RecalboxConf::Screensaver>& item, int id, int index,
                                     const RecalboxConf::Screensaver& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScreensaverType: mConf.SetScreenSaverType(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<RecalboxConf::Screensaver> !");
  }
}

void MenuProvider::MenuMultiChanged(const ItemSelectorBase<String>& item, int id, int index,
                                    const std::vector<String>& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ScreensaverSystems: mConf.SetScreenSaverSystemList(value).Save(); break;
    case MenuItemType::ArcadeManufacturerSystems: mConf.SetCollectionArcadeManufacturers(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<String> !");
  }
}

void MenuProvider::ReceiveSyncMessage(const ItemSelectorBase<ThemeSpec>* const& sourceItem)
{
  String name;
  (void)ThemeManager::Instance().AnalyseAndSwitch(mWindow, mLastSelectedTheme.FolderPath, name, true);
  // force hasdefaulttransition check to grey unneeded options
  ThemeManager::Instance().Main().getThemeTransition();
  ((MenuBuilder&)sourceItem->Parent()).RequestMenuRefresh();
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ThemeSpec>& item, int id, int index, const ThemeSpec& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::ThemeSet:
    {
      mLastSelectedTheme = value;
      //mConf.SetThemeFolder(value.Name).Save();
      mSender.SendDelayed(&item, 2);
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ThemeSpec> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<SystemSorting>& item, int id, int index,
                                     const SystemSorting& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::SystemSorting:
    {
      mConf.SetSystemSorting(value).Save();
      // TODO move the following code in ViewController in OnSystemSortingChanged() notification
      mSystemManager.SystemSorting();
      ViewController::Instance().getSystemListView().Sort();
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<SystemSorting> !");
  }
}

void MenuProvider::MenuMultiChanged(const ItemSelectorBase<GameGenres>& item, int id, int index,
                                    const std::vector<GameGenres>& value)
{
  (void)id;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::VirtualPerGenre:
    {
      // Build list & save
      String::List list;
      for (GameGenres gg: value) list.push_back(SystemManager::BuildGenreSystemName(gg));
      mConf.SetCollectionGenre(list).Save();

      // Refresh systems
      for (GameGenres ug: item.AllUnselectedValues())
        mSystemManager.UpdateVirtualGenreSystemsVisibility(ug, SystemManager::Visibility::Hide);
      for (GameGenres sg: item.AllSelectedValues())
        mSystemManager.UpdateVirtualGenreSystemsVisibility(sg, SystemManager::Visibility::ShowAndSelect);
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<GameGenres> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<Overclocking::Overclock>& item, int id, int index,
                                     const Overclocking::Overclock& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::Overclock:
    {
      MenuBuilder* parent = (MenuBuilder*)&item.Parent();
      if (value.Hazardous())
        mWindow.pushGui(new GuiMsgBox(mWindow, _("TURBO AND EXTREM OVERCLOCK PRESETS MAY CAUSE SYSTEM UNSTABILITIES, SO USE THEM AT YOUR OWN RISK.\nIF YOU CONTINUE, THE SYSTEM WILL REBOOT NOW."),
                        _("YES"), [value, parent] { Overclocking::Install(value); parent->RequestReboot(); }, _("NO"), nullptr));
      else { Overclocking::Install(value); parent->RequestReboot(); }
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<Overclocking::Overclock> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<CrtAdapterType>& item, int id, int index,
                                     const CrtAdapterType& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::CrtAdapter:
    {
      CrtAdapterType oldValue = Board::Instance().CrtBoard().GetCrtAdapter();
      if (value == CrtAdapterType::None && oldValue != CrtAdapterType::None)
      {
        CrtConf::Instance().SetSystemCRT(CrtAdapterType::None).Save();
        mConf.SetEmulationstationVideoMode("default").Save();
        CoItemAsList<String>(item, MenuItemType::CrtMenuResolution).SetSelectedItemValue("default", true);
      }
      CrtConf::Instance().SetSystemCRT(value).Save();
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<CrtAdapterType> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<ICrtInterface::HorizontalFrequency>& item, int id, int index, const ICrtInterface::HorizontalFrequency &value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::CrtScreenType:
    {
      mWindow.pushGui(new GuiMsgBox(mWindow,
                                    _("Make sure to check the compatibility of your hardware before changing the recalbox refresh rate. Are you sure you want to switch the display mode to ").Append(ICrtInterface::HorizontalFrequencyToHumanReadable(value)).Append("?"),
                                    _("CANCEL"), [&item] { item.SetSelectedItemValue(Board::Instance().CrtBoard().GetHorizontalFrequency(), false); },
                                    _("YES"), [value]{ CrtConf::Instance().SetSystemCRTScreenType((int)value).Save(); }));
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<ICrtInterface::HorizontalFrequency> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<CrtScanlines>& item, int id, int index, const CrtScanlines& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::CrtScanline240in480: mCrtConf.SetSystemCRTScanlines31kHz(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<CrtScanlines> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<AudioMode>& item, int id, int index, const AudioMode& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::SoundAudioMode: mConf.SetAudioMode(value).Save(); break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<AudioMode> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<String::Unicode>& item, int id, int index, const String::Unicode& value)
{
  (void)item;
  (void)index;
  (void)value;
  switch((MenuItemType)id)
  {
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<String::Unicode> !");
  }
}

void MenuProvider::MenuMultiChanged(const ItemSelectorBase<RecalboxConf::GamelistDecoration>& item, int id, int index,
                                    const std::vector<RecalboxConf::GamelistDecoration>& value)
{
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::DecorateGames:
    {
      assert(item.Context().HasSystem() && "No system context");
      RecalboxConf::GamelistDecoration decorations = RecalboxConf::GamelistDecoration::None;
      for (RecalboxConf::GamelistDecoration deco: value) decorations |= deco;
      mConf.SetSystemGamelistDecoration(*item.Context().System(), decorations).Save();
      break;
    }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<RecalboxConf::GamelistDecoration> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<FileSorts::Sorts>& item, int id, int index,
                                     const FileSorts::Sorts& value)
{
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::SortGames: { assert(item.Context().HasSystem() && "No system context"); mConf.SetSystemSort(*item.Context().System(), value).Save(); break; }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<FileSorts::Sorts> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<Path>& item, int id, int index, const Path& value)
{
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::MetadataSoftpatching: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetLastPatch(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::LastPatch, false); break; }
    case MenuItemType::SoftpatchingSelectPatch: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetLastPatch(value); break; }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<Path> !");
  }
}

void MenuProvider::MenuRatingMoved(ItemRating& item, int id, float value)
{
  switch((MenuItemType)id)
  {
    case MenuItemType::MetadataRating: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetRating(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::Rating, false); break; }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuRatingMoved !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<GameGenres>& item, int id, int index, const GameGenres& value)
{
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::MetadataGenre: { assert(item.Context().HasGame() && "No game context"); item.Context().Game()->Metadata().SetGenreId(value); mSystemManager.UpdateSystemsOnGameChange(item.Context().Game(), MetadataType::GenreId, false); break; }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<GameGenres> !");
  }
}

void MenuProvider::MenuMultiChanged(const ItemSelectorBase<Path>& item, int id, int index, const std::vector<Path>& value)
{
  (void)index;
  (void)value;
  (void)item;
  switch((MenuItemType)id)
  {
    case MenuItemType::DeleteSelectGameFiles:
    case MenuItemType::DeleteSelectGameMedia:
    case MenuItemType::DeleteSelectFileExtra:
    case MenuItemType::DeleteSelectFileSaves: break;
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuMultiChanged<Path> !");
  }
}

void MenuProvider::MenuSingleChanged(ItemSelectorBase<int>& item, int id, int index, const int& value)
{
  (void)item;
  (void)index;
  switch((MenuItemType)id)
  {
    case MenuItemType::NetplayUsePassword: { mConf.SetNetplayPasswordClient(value); break; }
    case MenuItemType::NetplaySelectPlayerPassword: { mConf.SetNetplayPasswordUseForPlayer(value).Save(); break; }
    case MenuItemType::NetplaySelectViewerPassword: { mConf.SetNetplayPasswordUseForViewer(value).Save(); break; }
    default:
      mWindow.displayMessage("[MenuProcessor] Unprocessed id " + MenuConverters::ItemToString((MenuItemType)id) + " in MenuSingleChanged<int> !");
  }

}

