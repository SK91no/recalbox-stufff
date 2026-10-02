//
// Created by matthieu on 12/09/15.
//
#pragma once

#include <utils/IniFile.h>
#include <utils/String.h>
#include <utils/cplusplus/StaticLifeCycleControler.h>
#include <games/FileSorts.h>
#include <scraping/ScraperTools.h>
#include <scraping/scrapers/screenscraper/ScreenScraperEnums.h>
#include <scraping/scrapers/screenscraper/Languages.h>
#include <audio/AudioMode.h>
#include "systems/SystemSorting.h"
#include "IRecalboxConfChanged.h"
#include <scraping/ScraperType.h>

// Forward declaration
class SystemData;

class RecalboxConf: public IniFile, public StaticLifeCycleControler<RecalboxConf>
{
  public:
    /*!
     * @brief Constructor
     */
    RecalboxConf();

    //! Virtual destructor
    ~RecalboxConf() override = default;

    /*!
     * @brief Called when file has been saved
     */
    void OnSave() const override;

    /*
     * Watching
     */

    void Watch(const String& key, IRecalboxConfChanged& callback);

    /*
     * Enums
     */

    enum class UpdateType
    {
      Stable,     //!< Stable release
      Patron,     //!< Patreon versions
      Alpha,      //!< Alpha versions
      RRGBD2,
      JVS,
      PatronAlpha,
    };

    enum class Menu
    {
        Default, //!< All menu available
        Bartop,  //!< Limited menu
        None,    //!< No menu except exit
    };

    enum class Transition
    {
      Slide,    //!< Slide animation
      Fade,     //!< Fade
      Instant,  //!< No transition at all
    };

    enum class Screensaver
    {
      Black,    //!< Black screen
      Dim,      //!< 50% luminosity
      Demo,     //!< Real game demos
      Gameclip, //!< Game clips
      Suspend,  //!< Hardware suspend
      Bouncing, //!< Bouncing logo
    };

    enum class Relay
    {
        None,     //!< No relay
        NewYork,  //!< New-york relay
        Madrid,   //!< Madrid relay
        Montreal, //!< Montreal relay
        Saopaulo, //!< SaoPaulo relay
    };

    enum class SoftPatching
    {
      Disable, //!< No soft patching
      Auto,    //!< Automatic patch selection
      LaunchLast,    //!< Automatic patch selection
      Select,  //!< Manual patch selection
    };

    enum class PadOSDType
    {
      Snes, //!< SNES type pad
      MD  , //!< Megadrive type pad
      XBox, //!< XBox type pad
      PSX , //!< Playstation type pad
      N64 , //!< Nintendo 64 type pad
      DC  , //!< Dreamcast pad type
    };

    enum class GamelistDecoration
    {
      None          = 0, //!< Nothing
      Regions       = 1, //!< Region flags
      Players       = 2, //!< Players
      Genre         = 4, //!< normalized genre
      SupportNumber = 8, //!< Support numbers
      SupportType   = 16,//!< Support type
    };

    enum class GamelistLayout
    {
      Legacy,   //!< Legacy text list
      LogoList, //!< Logo vertical list
      Carousel, //!< Horizontal box carousel
      Grid,     //!< Grid of boxes
      Default,  //!< No layout of its own: follow the global layout
    };
    //! On-screen virtual keyboard variant chosen by the user
    enum class VirtualKeyboardType
    {
      Wheel,      //!< Arcade rotating wheel (legacy default)
      Classic,    //!< Classic QWERTY/AZERTY/... on-screen keyboard
      Simplified, //!< Paged real keyboard (lowercase / uppercase / symbols), L1/R1 to switch
    };


    /*
     * Shortcuts
     */

    #define DoNotify(name, value) do{ for(I##name##Notification* p : mArrayOf##name##Notify) p->Configuration##name##Changed(value); }while(false)

    #define DefineObservable(clazz, name, type) \
      public: \
        class I##name##Notification; \
      private: \
        Array<I##name##Notification*> mArrayOf##name##Notify; \
      public: \
        void Add##name##ConfigurationNotification(I##name##Notification* notified) { mArrayOf##name##Notify.Add(notified); } \
        void Remove##name##ConfigurationNotification(I##name##Notification* notified) { mArrayOf##name##Notify.Remove(notified); } \
        class I##name##Notification \
        { \
          public: \
            I##name##Notification() { clazz::Instance().Add##name##ConfigurationNotification(this); }; \
            virtual ~I##name##Notification() { clazz::Instance().Remove##name##ConfigurationNotification(this); }; \
            virtual void Configuration##name##Changed(const type& value) = 0; \
        };

    #define DoNotifySystem(name, system, value) do{ for(ISystem##name##Notification* p : mArrayOfSystem##name##Notify) p->ConfigurationSystem##name##Changed(system, value); }while(false)

    #define DefineSystemObservable(clazz, name, type) \
      public: \
        class ISystem##name##Notification; \
      private: \
        Array<ISystem##name##Notification*> mArrayOf##System##name##Notify; \
      public: \
        void AddSystem##name##ConfigurationNotification(ISystem##name##Notification* notified) { mArrayOfSystem##name##Notify.Add(notified); } \
        void RemoveSystem##name##ConfigurationNotification(ISystem##name##Notification* notified) { mArrayOfSystem##name##Notify.Remove(notified); } \
        class ISystem##name##Notification \
        { \
          public: \
            ISystem##name##Notification() { clazz::Instance().AddSystem##name##ConfigurationNotification(this); }; \
            virtual ~ISystem##name##Notification() { clazz::Instance().RemoveSystem##name##ConfigurationNotification(this); }; \
            virtual void ConfigurationSystem##name##Changed(const SystemData& system, const type& value) = 0; \
        };

    #define DefineGetterSetterGeneric(clazz, name, type, type2, key, defaultValue) \
      type Get##name() const { return As##type2(key, defaultValue); } \
      clazz& Delete##name() { Delete(key); return *this; } \
      bool IsDefined##name() const { return IsDefined(key); } \
      clazz& Set##name(const type& value) { Set##type2(key, value); DoNotify(name, value); return *this; } \
      bool Has##name() const { return HasKey(key); } \
      const type& Get##name##Default() const { static type s(defaultValue); return s; } \
      DefineObservable(clazz, name, type)

    // Note: dispatch self-registration lives in the RecalboxConf-specific wrappers below, not in the
    // *Generic macros, because the latter are shared with CrtConf (which has no dispatch registry).
    #define DefineGetterSetter(name, type, type2, key, defaultValue) \
      DefineGetterSetterGeneric(RecalboxConf, name, type, type2, key, defaultValue) \
      static void ApplyByKey##name(const String& v) { Instance().Set##name(Parse##type2(v)); } \
      static void BindByKey##name() { GlobalDispatchRegistry()[key] = &ApplyByKey##name; } \
      static inline DispatchRegistrar sBindByKey##name{ &BindByKey##name };

    #define DefineGetterSetterIndexed(clazz, name, type, type2, key, defaultValue) \
      type Get##name(int index) const { return As##type2(String(key).Append('.').Append(index), defaultValue); } \
      clazz& Delete##name(int index) { Delete(String(key).Append('.').Append(index)); return *this; } \
      bool IsDefined##name(int index) const { return IsDefined(String(key).Append('.').Append(index)); } \
      clazz& Set##name(int index, const type& value) { Set##type2(String(key).Append('.').Append(index), value); return *this; }

    #define DefineListGetterSetter(name, key, defaultValue) \
      String::List Get##name() const { return AsString(key, defaultValue).Split(','); } \
      String GetRaw##name() const { return AsString(key, defaultValue); } \
      bool IsIn##name(const String& value) const { return isInList(key, value); } \
      RecalboxConf& Set##name(const String::List& value) { SetString(key, String::Join(value, ',')); DoNotify(name, value); return *this; } \
      static void ApplyByKey##name(const String& v) { Instance().Set##name(v.Split(',')); } \
      static void BindByKey##name() { GlobalDispatchRegistry()[key] = &ApplyByKey##name; } \
      static inline DispatchRegistrar sBindByKey##name{ &BindByKey##name }; \
      DefineObservable(RecalboxConf, name, String::List)

    #define DefineGetterSetterParameterized(name, type, type2, keybefore, keyafter, defaultValue) \
      type Get##name(const String& subkey) const { return As##type2(String(keybefore).Append(subkey).Append(keyafter), defaultValue); } \
      RecalboxConf& Set##name(const String& subkey, const type& value) { Set##type2(String(keybefore).Append(subkey).Append(keyafter), value); return *this; }

    #define DefineGetterSetterParameterized2(name, type, type2, keystart, defaultValue) \
      type Get##name(const String& subkey, const String& endkey) const { return As##type2(String(keystart).Append(subkey).Append('.').Append(endkey), defaultValue); } \
      RecalboxConf& Set##name(const String& subkey, const String& endkey, const type& value) { Set##type2(String(keystart).Append(subkey).Append('.').Append(endkey), value); return *this; }

    #define DefineSystemGetterSetterImplementation(name, type, type2, key, defaultValue, neutralValue) \
      type RecalboxConf::GetSystem##name(const SystemData& system) const { return As##type2(String(system.Name()).Append('.').Append(key), defaultValue); } \
      type RecalboxConf::GetSystem##name##NoDefault(const SystemData& system) const { return As##type2(String(system.Name()).Append('.').Append(key), neutralValue); } \
      RecalboxConf& RecalboxConf::SetSystem##name(const SystemData& system, const type& value) { Set##type2(String(system.Name()).Append('.').Append(key), value); DoNotifySystem(name, system, value); return *this; } \
      RecalboxConf& RecalboxConf::DeleteSystem##name(const SystemData& system) { Delete(String(system.Name()).Append('.').Append(key)); return *this; } \
      bool RecalboxConf::IsDefinedSystem##name(const SystemData& system) const { return IsDefined(String(system.Name()).Append('.').Append(key)); } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { RecalboxConf::Instance().SetSystem##name(s, RecalboxConf::Parse##type2(v)); } \
      static RecalboxConf::SystemDispatchRegistrar sSysBind##name { RecalboxConf::key, &SysApplyByKey##name };

    #define DefineEmulationStationSystemGetterSetterImplementation(name, type, type2, key, defaultValue) \
      type RecalboxConf::GetSystem##name(const SystemData& system) const { return As##type2(String("emulationstation.").Append(system.Name()).Append('.').Append(key), defaultValue); } \
      RecalboxConf& RecalboxConf::SetSystem##name(const SystemData& system, const type& value) { Set##type2(String("emulationstation.").Append(system.Name()).Append('.').Append(key), value); DoNotifySystem(name, system, value); return *this; } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { RecalboxConf::Instance().SetSystem##name(s, RecalboxConf::Parse##type2(v)); } \
      static RecalboxConf::SystemDispatchRegistrar sSysBind##name { RecalboxConf::key, &SysApplyByKey##name };

    #define DefineSystemGetterSetterDeclaration(name, type, type2, key) \
      type GetSystem##name(const SystemData& system) const; \
      type GetSystem##name##NoDefault(const SystemData& system) const; \
      RecalboxConf& SetSystem##name(const SystemData& system, const type& value); \
      RecalboxConf& DeleteSystem##name(const SystemData& system); \
      bool IsDefinedSystem##name(const SystemData& system) const; \
      DefineSystemObservable(RecalboxConf, name, type)

    #define DefineEmulationStationSystemGetterSetterDeclaration(name, type, type2, key) \
      type GetSystem##name(const SystemData& system) const; \
      RecalboxConf& SetSystem##name(const SystemData& system, const type& value); \
      DefineSystemObservable(RecalboxConf, name, type)

    #define DefineGetterSetterEnumGeneric(clazz, name, enumType, key, adapterPrefix) \
      enumType Get##name() const { return adapterPrefix##FromString(AsString(key, "")); } \
      clazz& Set##name(enumType value) { SetString(key, adapterPrefix##FromEnum(value)); DoNotify(name, value); return *this; } \
      DefineObservable(clazz, name, enumType)

    #define DefineGetterSetterEnum(name, enumType, key, adapterPrefix) \
       DefineGetterSetterEnumGeneric(RecalboxConf, name, enumType, key, adapterPrefix) \
       static void ApplyByKey##name(const String& v) { Instance().Set##name(adapterPrefix##FromString(v)); } \
       static void BindByKey##name() { GlobalDispatchRegistry()[key] = &ApplyByKey##name; } \
       static inline DispatchRegistrar sBindByKey##name{ &BindByKey##name };

    #define DefineEmulationStationSystemGetterSetterNumericEnumDeclaration(name, enumType) \
      enumType GetSystem##name(const SystemData& system) const; \
      RecalboxConf& SetSystem##name(const SystemData& system, enumType value); \
      DefineSystemObservable(RecalboxConf, name, enumType)

    #define DefineEmulationStationSystemGetterSetterNumericEnumImplementation(name, enumType, key, defaultValue) \
      enumType RecalboxConf::GetSystem##name(const SystemData& system) const { return (enumType)AsInt(String("emulationstation.").Append(system.Name()).Append('.').Append(key), (int)(defaultValue)); } \
      RecalboxConf& RecalboxConf::SetSystem##name(const SystemData& system, enumType value) { SetInt(String("emulationstation.").Append(system.Name()).Append('.').Append(key), (int)value);  DoNotifySystem(name, system, value); return *this; } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { RecalboxConf::Instance().SetSystem##name(s, (enumType)RecalboxConf::ParseInt(v)); } \
      static RecalboxConf::SystemDispatchRegistrar sSysBind##name { RecalboxConf::key, &SysApplyByKey##name };

    #define DefineEmulationStationSystemGetterSetterEnumDeclaration(name, enumType) \
      enumType GetSystem##name(const SystemData& system) const; \
      RecalboxConf& SetSystem##name(const SystemData& system, enumType value); \
      DefineSystemObservable(RecalboxConf, name, enumType)

    #define DefineEmulationStationSystemGetterSetterEnumImplementation(clazz, name, enumType, key, adapterPrefix) \
      clazz::enumType clazz::GetSystem##name(const SystemData& system) const { return adapterPrefix##FromString(AsString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), "")); } \
      clazz& clazz::SetSystem##name(const SystemData& system, enumType value) { SetString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), adapterPrefix##FromEnum(value)); DoNotifySystem(name, system, value); return *this; } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { clazz::Instance().SetSystem##name(s, clazz::adapterPrefix##FromString(v)); } \
      static clazz::SystemDispatchRegistrar sSysBind##name { key, &SysApplyByKey##name };

    /*
     * Same as the two macros above, except an undefined key means "follow the global
     * setting", the way the per-system emulator settings do: Get##name() resolves the
     * inheritance while Get##name##NoDefault() returns the neutral value, so callers can
     * tell both apart, and setting the neutral value removes the key instead of writing it.
     */
    #define DefineEmulationStationSystemGetterSetterEnumInheritedDeclaration(name, enumType) \
      enumType GetSystem##name(const SystemData& system) const; \
      enumType GetSystem##name##NoDefault(const SystemData& system) const; \
      RecalboxConf& SetSystem##name(const SystemData& system, enumType value); \
      RecalboxConf& DeleteSystem##name(const SystemData& system); \
      bool IsDefinedSystem##name(const SystemData& system) const; \
      DefineSystemObservable(RecalboxConf, name, enumType)

    #define DefineEmulationStationSystemGetterSetterEnumInheritedImplementation(clazz, name, enumType, key, adapterPrefix, inheritedValue, neutralValue) \
      clazz::enumType clazz::GetSystem##name(const SystemData& system) const { enumType value = GetSystem##name##NoDefault(system); return value != (neutralValue) ? value : (inheritedValue); } \
      clazz::enumType clazz::GetSystem##name##NoDefault(const SystemData& system) const { return adapterPrefix##FromString(AsString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), "")); } \
      clazz& clazz::SetSystem##name(const SystemData& system, enumType value) { if (value == (neutralValue)) return DeleteSystem##name(system); SetString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), adapterPrefix##FromEnum(value)); DoNotifySystem(name, system, value); return *this; } \
      clazz& clazz::DeleteSystem##name(const SystemData& system) { Delete(String("emulationstation.").Append(system.Name()).Append('.').Append(key)); enumType inherited = GetSystem##name(system); DoNotifySystem(name, system, inherited); return *this; } \
      bool clazz::IsDefinedSystem##name(const SystemData& system) const { return IsDefined(String("emulationstation.").Append(system.Name()).Append('.').Append(key)); } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { clazz::Instance().SetSystem##name(s, clazz::adapterPrefix##FromString(v)); } \
      static clazz::SystemDispatchRegistrar sSysBind##name { key, &SysApplyByKey##name };

    #define DefineEmulationStationSystemListGetterSetterDeclaration(name, key) \
      String::List Get##name(const SystemData& system) const; \
      bool IsIn##name(const SystemData& system, const String& value) const; \
      RecalboxConf& Set##name(const SystemData& system, const String::List& value); \
      DefineSystemObservable(RecalboxConf, name, String::List)

    #define DefineEmulationStationSystemListGetterSetterImplementation(name, key, defaultValue) \
      String::List RecalboxConf::Get##name(const SystemData& system) const { return AsString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), defaultValue).Split(','); } \
      bool RecalboxConf::IsIn##name(const SystemData& system, const String& value) const { return isInList(String("emulationstation.").Append(system.Name()).Append('.').Append(key), value); } \
      RecalboxConf& RecalboxConf::Set##name(const SystemData& system, const String::List& value) { SetString(String("emulationstation.").Append(system.Name()).Append('.').Append(key), String::Join(value, ',')); DoNotifySystem(name, system, value); return *this; } \
      static void SysApplyByKey##name(SystemData& s, const String& v) { RecalboxConf::Instance().Set##name(s, v.Split(',')); } \
      static RecalboxConf::SystemDispatchRegistrar sSysBind##name { RecalboxConf::key, &SysApplyByKey##name };

    /*
     * String-keyed dispatch infrastructure (web manager bridge)
     *
     * Each in-class setter macro self-registers a (key -> typed-setter thunk) entry into this
     * registry, so SetByKey() is a pure lookup with no maintained list. Registration happens at
     * static-init through BindByKey##name(), whose body is a complete-class context: that is what
     * lets it reference the key constants declared further down in the class.
     */
    using StringSetterThunk = void(*)(const String&);
    static HashMap<String, StringSetterThunk>& GlobalDispatchRegistry() { static HashMap<String, StringSetterThunk> registry; return registry; }
    struct DispatchRegistrar { explicit DispatchRegistrar(void(*bind)()) { bind(); } };
    static bool   ParseBool(const String& v)   { return v.AsBool(); }
    static int    ParseInt(const String& v)    { int r = 0; (void)v.TryAsInt(r); return r; }
    static String ParseString(const String& v) { return v; }

    // System-specific dispatch: keyed by short key (e.g. "sort", "ratio"); the thunk resolves the system.
    // Registrars are emitted at namespace scope by the *Implementation macros in the .cpp, where all key
    // constants and converters are visible, so registration is direct (no complete-class-context trick needed).
    using SystemSetterThunk = void(*)(SystemData&, const String&);
    static HashMap<String, SystemSetterThunk>& SystemDispatchRegistry() { static HashMap<String, SystemSetterThunk> registry; return registry; }
    struct SystemDispatchRegistrar { SystemDispatchRegistrar(const char* key, SystemSetterThunk fn) { SystemDispatchRegistry()[key] = fn; } };

    DefineEmulationStationSystemListGetterSetterDeclaration(ArcadeSystemHiddenManufacturers, sArcadeSystemHiddenManufacturers)

    DefineGetterSetterEnum(MenuType, Menu, sMenuType, Menu)
    DefineGetterSetterEnum(ScraperNameOptions, ScraperNameOptions, sScraperGetNameFrom, ScraperTools::ScraperNameOptions)
    DefineGetterSetterEnum(ScreenScraperRegionPriority, ScreenScraperEnums::ScreenScraperRegionPriority, sScreenScraperRegionPriority, ScreenScraperEnums::ScreenScraperRegionPriority)
    DefineGetterSetterEnum(ScreenScraperRegion, Regions::GameRegions, sScreenScraperRegion, Regions::GameRegions)
    DefineGetterSetterEnum(ScreenScraperLanguage, Languages, sScreenScraperLanguage, LanguagesTools::Language)
    DefineGetterSetterEnum(ScreenScraperMainMedia, ScreenScraperEnums::ScreenScraperImageType, sScreenScraperMainMedia, ScreenScraperEnums::ScreenScraperImageType)
    DefineGetterSetterEnum(ScreenScraperBox, ScreenScraperEnums::ScreenScraperBoxType, sScreenScraperBox, ScreenScraperEnums::ScreenScraperBoxType)
    DefineGetterSetterEnum(ScreenScraperVideo, ScreenScraperEnums::ScreenScraperVideoType, sScreenScraperVideo, ScreenScraperEnums::ScreenScraperVideoType)
    DefineGetterSetterEnum(AudioMode, AudioMode, sAudioOptions, AudioMode)
    DefineGetterSetterEnum(SystemSorting, SystemSorting, sSystemSorting, SystemSorting)
    DefineGetterSetterEnum(UpdateType, UpdateType, sUpdateType, UpdateType)
    DefineGetterSetterEnum(RomPreferredRegion, Regions::GameRegions, sRomPreferredRegion, Regions::GameRegions)
    DefineGetterSetterEnum(RomSecondPreferredRegion, Regions::GameRegions, sRomSecondPreferredRegion, Regions::GameRegions)
    DefineGetterSetterEnum(RomPreferredFallback, Regions::OrderedMasterRegions, sRomPreferredFallback, Regions::OrderedMasterRegions)

    DefineGetterSetter(DebugLogs, bool, Bool, sDebugLogs, false)

    DefineGetterSetter(Hostname, String, String, sHostname, "RECALBOX")

    DefineGetterSetter(WifiEnabled, bool, Bool, sWifiEnabled, false)
    DefineGetterSetter(WifiConnect, bool, Bool, sWifiConnect, true)
    DefineGetterSetter(WifiSSID, String, String, sWifiSSID, "")
    DefineGetterSetter(WifiKey, String, String, sWifiKey, "")

    DefineGetterSetter(SwapValidateAndCancel, bool, Bool, sSwapValidateAndCancel, false)

    DefineGetterSetter(AudioVolume, int, Int, sAudioVolume, 90)
    DefineGetterSetter(AudioMusicVolume, int, Int, sAudioMusicVolume, 35)
    DefineGetterSetter(AudioOuput, String, String, sAudioOuput, "")

    DefineGetterSetter(MusicRemoteEnable, bool, Bool, sMusicDisableRemote, true)

    DefineGetterSetter(ScreenSaverTime, int, Int, sScreenSaverTime, 5)
    DefineGetterSetterEnum(ScreenSaverType, Screensaver, sScreenSaverType, Screensaver)
    DefineListGetterSetter(ScreenSaverSystemList, sScreenSaverSystemList, "")

    DefineGetterSetter(PopupHelp, int, Int, sPopupHelp, 10)
    DefineGetterSetter(PopupMusic, int, Int, sPopupMusic, 5)
    DefineGetterSetter(PopupNetplay, int, Int, sPopupNetplay, 8)

    DefineGetterSetter(ThemeCarousel, bool, Bool, sThemeCarousel, 1)
    DefineGetterSetterEnum(ThemeTransition, Transition, sThemeTransition, Transition)
    DefineGetterSetterEnum(GameTransition, Transition, sGameTransition, Transition)
    DefineGetterSetter(ThemeRegion, String, String, sThemeRegion, "us")
    DefineGetterSetter(ThemeFolder, String, String, sThemeFolder, "recalbox-next")

    DefineGetterSetterParameterized(ThemeColorSet    , String, String, sThemeGeneric, ".colorset", "")
    DefineGetterSetterParameterized(ThemeIconSet     , String, String, sThemeGeneric, ".iconset", "")
    DefineGetterSetterParameterized(ThemeMenuSet     , String, String, sThemeGeneric, ".menuset", "")
    DefineGetterSetterParameterized(ThemeSystemView  , String, String, sThemeGeneric, ".systemview", "")
    DefineGetterSetterParameterized(ThemeGamelistView, String, String, sThemeGeneric, ".gamelistview", "")
    DefineGetterSetterParameterized(ThemeGameClipView, String, String, sThemeGeneric, ".gameclipview", "")
    // All 6 nopn-generic options above are deprecated - Use the new generic ThemeOption
    DefineGetterSetterParameterized2(ThemeOption, String, String, sThemeGeneric, "")

    DefineGetterSetterParameterized2(CardBoot, String, String, sCardReaderGeneric, "")

    DefineGetterSetter(Brightness, int, Int, sBrightness, 7)
    DefineGetterSetter(Clock, bool, Bool, sClock, false)
    DefineGetterSetter(ShowHelp, bool, Bool, sShowHelp, true)
    DefineGetterSetter(ShowGameClipHelpItems, bool, Bool, sShowGameClipHelpItems, true)
    DefineGetterSetter(ShowGameClipClippingItem, bool, Bool, sShowGameClipClippingItem, true)
    DefineGetterSetter(QuickSystemSelect, bool, Bool, sQuickSystemSelect, true)
    DefineGetterSetter(ListFastMoveEnabled, bool, Bool, sListFastMoveEnabled, true)
    DefineGetterSetter(FilterAdultGames, bool, Bool, sFilterAdultGames, true)
    DefineGetterSetter(EnableEditFavorites, bool, Bool, sEnableEditFavorites, true)
    DefineGetterSetter(FavoritesOnly, bool, Bool, sFavoritesOnly, false)
    DefineGetterSetter(FavoritesFirst, bool, Bool, sFavoritesFirst, false)
    DefineGetterSetter(ShowHidden, bool, Bool, sShowHidden, false)
    DefineGetterSetter(OneGameOneRom, bool, Bool, sOneGameOneRom, false)
    DefineGetterSetter(DisplayByFileName, bool, Bool, sDisplayByFileName, false)
    DefineGetterSetter(ShowOnlyLatestVersion, bool, Bool, sShowOnlyLatestVersion, false)
    DefineGetterSetter(HideNoGames, bool, Bool, sHideNoGames, false)
    DefineGetterSetter(HideBoardGames, bool, Bool, sHideBoardGames, false)
    DefineGetterSetter(ShowOnly3PlusPlayers, bool, Bool, sShowOnly3PlusPlayers, false)

    DefineGetterSetter(FirstTimeUse, bool, Bool, sFirstTimeUse, true)
    DefineGetterSetter(FirstInstallDual2, bool, Bool, sFirstTimeDual2, true)
    DefineGetterSetter(FirstInstallJamma2, bool, Bool, sFirstTimeJamma2, true)
    DefineGetterSetter(FirstInstallJvs, bool, Bool, sFirstTimeJvs, true)
    DefineGetterSetter(FirstInstallCardReader, bool, Bool, sFirstTimeCardReader, true)

    DefineGetterSetter(SystemLanguage, String, String, sSystemLanguage, "en_US")
    DefineGetterSetter(SystemTimezone, String, String, sSystemTimezone, "UTC")
    DefineGetterSetter(SystemKbLayout, String, String, sSystemKbLayout, "us")
    DefineGetterSetterEnum(VirtualKeyboardType, VirtualKeyboardType, sVirtualKeyboardType, VirtualKeyboardType)
    DefineGetterSetter(SystemManagerEnabled, bool, Bool, sSystemManagerEnabled, true)

    DefineGetterSetter(Overclocking, String, String, sOverclocking, "none")

    DefineGetterSetter(KodiEnabled, bool, Bool, sKodiEnabled, true)
    DefineGetterSetter(KodiAtStartup, bool, Bool, sKodiAtStartup, false)
    DefineGetterSetter(KodiXButton, bool, Bool, sKodiXButton, false)

    DefineGetterSetterEnum(ScraperSource, ScraperType, sScraperSource, ScraperType)
    DefineGetterSetter(ScraperAuto, bool, Bool, sScraperAuto, true)

    DefineGetterSetter(RecalboxPrivateKey, String, String, sRecalboxPrivateKey, "")

    DefineGetterSetter(ScreenScraperLogin, String, String, sScreenScraperLogin, "")
    DefineGetterSetter(ScreenScraperPassword, String, String, sScreenScraperPassword, "")
    DefineGetterSetterEnum(ScreenScraperLogo, ScreenScraperEnums::ScreenScraperLogoType, sScreenScraperLogo, ScreenScraperEnums::ScreenScraperLogoType)
    DefineGetterSetter(ScreenScraperWantManual, bool, Bool, sScreenScraperWantManual, false)
    DefineGetterSetter(ScreenScraperWantMaps, bool, Bool, sScreenScraperWantMaps, false)
    DefineGetterSetter(ScreenScraperWantTips, bool, Bool, sScreenScraperWantTips, false)
    DefineGetterSetter(ScreenScraperWantP2K, bool, Bool, sScreenScraperWantP2K, false)

    DefineGetterSetter(NetplayEnabled, bool, Bool, sNetplayEnabled, false)
    DefineGetterSetter(NetplayLogin, String, String, sNetplayLogin, "")
    DefineGetterSetter(NetplayLobby, String, String, sNetplayLobby, "http://lobby.libretro.com/list/")
    DefineGetterSetter(NetplayPort, int, Int, sNetplayPort, sNetplayDefaultPort)
    DefineGetterSetterEnum(NetplayRelay, Relay, sNetplayRelay, Relay)

    DefineGetterSetter(RetroAchievementOnOff, bool, Bool, sRetroAchievementOnOff, false)
    DefineGetterSetter(RetroAchievementHardcore, bool, Bool, sRetroAchievementHardcore, false)
    DefineGetterSetter(RetroAchievementLogin, String, String, sRetroAchievementLogin, "")
    DefineGetterSetter(RetroAchievementPassword, String, String, sRetroAchievementPassword, "")
    DefineGetterSetter(RetroAchievementSounds, bool, Bool, sRetroAchievementSounds, false)

    DefineGetterSetter(StartupGamelistOnly, bool, Bool, sStartupGamelistOnly, false)
    DefineGetterSetter(StartupSelectedSystem, String, String, sStartupSelectedSystem, "")
    DefineGetterSetter(StartupStartOnGamelist, bool, Bool, sStartupStartOnGamelist, false)
    DefineGetterSetter(StartupHideSystemView, bool, Bool, sStartupHideSystemView, false)

    DefineGetterSetter(PowerSwitch, String, String, sPowerSwitch, "")

    DefineGetterSetter(GlobalRatio, String, String, sGlobalRatio, "auto")
    DefineGetterSetter(GlobalSmooth, bool, Bool, sGlobalSmooth, true)
    DefineGetterSetter(GlobalRecalboxOverlays, bool, Bool, sGlobalRecalboxOverlays, true)
    DefineGetterSetter(GlobalRewind, bool, Bool, sGlobalRewind, true)
    DefineGetterSetter(GlobalAutoSave, bool, Bool, sGlobalAutoSave, false)
    DefineGetterSetter(GlobalQuitTwice, bool, Bool, sGlobalQuitTwice, false)
    DefineGetterSetter(GlobalHidePreinstalled, bool, Bool, sGlobalHidePreinstalled, false)
    DefineGetterSetter(GlobalIntegerScale, bool, Bool, sGlobalIntegerScale, false)
    DefineGetterSetterEnum(GlobalSoftpatching, SoftPatching, sGlobalSoftpatching, SoftPatching)
    DefineGetterSetter(GlobalShaders, String, String, sGlobalShaders, "")
    DefineGetterSetter(GlobalShaderSet, String, String, sGlobalShaderSet, "none")
    DefineGetterSetter(GlobalShowFPS, bool, Bool, sGlobalShowFPS, false)
    DefineGetterSetter(GlobalInputDriver, String, String, sGlobalInputDriver, "auto")
    DefineGetterSetter(GlobalDemoDuration, int, Int, sGlobalDemoDuration, 90)
    DefineGetterSetter(GlobalDemoInfoScreen, int, Int, sGlobalDemoInfoScreen, 6)
    DefineGetterSetter(GlobalReduceLatency, bool, Bool, sGlobalReduceLatency, false)
    DefineGetterSetter(GlobalRunAhead, bool, Bool, sGlobalRunAhead, false)
    DefineGetterSetter(GlobalShowSaveStateBeforeRun, bool, Bool, sGlobalShowSaveStateBeforeRun, false)
    DefineGetterSetter(GlobalHDMode, bool, Bool, sGlobalHDMode, false)
    DefineGetterSetter(GlobalAutoBlitter, String, String, sGlobalAutoBlitter, "recalbox")
    DefineGetterSetter(GlobalWidescreenMode, bool, Bool, sGlobalWidescreen, false)
    DefineGetterSetter(GlobalVulkanDriver, bool, Bool, sGlobalVulkanDriver, false)
    DefineGetterSetter(GlobalCrtHandheldFormat, String, String, sGlobalCrtHandheldFormat, "fullscreen")

    DefineGetterSetter(CollectionLastPlayed, bool, Bool, sCollectionLastPlayed, false)
    DefineGetterSetter(CollectionMultiplayer, bool, Bool, sCollectionMultiplayer, false)
    DefineGetterSetter(CollectionAllGames, bool, Bool, sCollectionAllGames, false)
    DefineGetterSetter(CollectionLightGun, bool, Bool, sCollectionLightGun, false)
    DefineGetterSetter(CollectionPorts, bool, Bool, sCollectionPorts, true)
    DefineGetterSetter(CollectionTate, bool, Bool, sCollectionTate, false)
    DefineGetterSetter(CollectionDial, bool, Bool, sCollectionDial, false)
    DefineGetterSetter(CollectionTrackball, bool, Bool, sCollectionTrackball, false)
    DefineListGetterSetter(CollectionGenre, sCollectionGenre, "")
    DefineGetterSetter(TateGameRotation, int, Int, sTateGameRotation, 0)
    DefineGetterSetter(ShowOnlyTateGames, bool, Bool, sTateOnly, false)
    DefineGetterSetter(ShowOnlyYokoGames, bool, Bool, sYokoOnly, false)

    DefineListGetterSetter(CollectionArcadeManufacturers, sCollectionArcadeManufacturers, "")
    DefineGetterSetter(CollectionArcade, bool, Bool, sCollectionArcade, false)
    DefineGetterSetter(CollectionArcadeHideOriginals, bool, Bool, sCollectionArcadeHideOriginals, true)
    DefineGetterSetter(CollectionArcadePosition, int, Int, sCollectionArcadePosition, 0)

    DefineGetterSetter(ArcadeUseDatabaseNames, bool, Bool, sArcadeUseDatabaseNames, true)
    DefineGetterSetter(ArcadeViewEnhanced, bool, Bool, sArcadeViewEnhanced, true)
    DefineGetterSetter(ArcadeViewHideBios, bool, Bool, sArcadeViewHideBios, false)
    DefineGetterSetter(ArcadeViewFoldClones, bool, Bool, sArcadeViewFoldClones, false)
    DefineGetterSetter(ArcadeViewHideNonWorking, bool, Bool, sArcadeViewHideNonWorking, false)

    DefineGetterSetter(UpdatesEnabled, bool, Bool, sUpdatesEnabled, true)


    DefineGetterSetter(EmulationstationVideoMode, String, String, sEsVideoMode, "")
    DefineGetterSetter(GlobalVideoMode, String, String, sGlobalVideoMode, "")
    DefineGetterSetter(KodiVideoMode, String, String, sKodiVideoMode, "")
    DefineGetterSetter(ESForce43, bool, Bool, sESForce43, false)
    DefineGetterSetter(SplashEnabled, bool, Bool, sSplashEnabled, true)
    DefineGetterSetter(DarkMode, bool, Bool, sDarkMode, false)


    DefineGetterSetter(BatteryHidden, bool, Bool, sBatteryHidden, false)
    DefineGetterSetter(PadOSD, bool, Bool, sPadOSD, false)
    DefineGetterSetterEnum(PadOSDType, PadOSDType, sPadOSDType, PadOSDType)
    DefineGetterSetter(AutoPairOnBoot, bool, Bool, sAutoPairOnBoot, true)
    DefineGetterSetter(ControllersRumble, bool, Bool, sControllersRumble, false)
    DefineListGetterSetter(GamepadProxyFilters, sGamepadProxyFilters, "")

    DefineGetterSetter(SuperGameBoy, String, String, sSuperGameBoyOption, "gb")
    DefineGetterSetter(Experimental, bool, Bool, sExperimental, GetUpdateType() != UpdateType::Stable)

    DefineGetterSetter(BootOnGameEnabled, bool, Bool, sBootOnGameEnabled, true)
    DefineGetterSetter(BootOnGameSystemUUID, String, String, sBootOnGameSystemUUID, "")
    DefineGetterSetter(BootOnGameGamePath, String, String, sBootOnGameGamePath, "")

    DefineGetterSetterIndexed(RecalboxConf, NetplayPasswords, String, String, sNetplayPasswords, "")
    DefineGetterSetter(NetplayPasswordUseForPlayer, bool, Bool, sNetplayPasswordUseForPlayer, false)
    DefineGetterSetter(NetplayPasswordUseForViewer, bool, Bool, sNetplayPasswordUseForViewer, false)
    DefineGetterSetter(NetplayPasswordLastForPlayer, int, Int, sNetplayPasswordLastForPlayer, 0)
    DefineGetterSetter(NetplayPasswordLastForViewer, int, Int, sNetplayPasswordLastForViewer, 0)
    DefineGetterSetter(NetplayPasswordClient, int, Int, sNetplayPasswordClient, -1)

    DefineGetterSetter(CRConsoleMode, bool, Bool, sCRConsoleMode, false)
    DefineGetterSetter(CRConsoleModeExit, String, String, sCRConsoleModeExit, "hkstart")
    DefineGetterSetter(CRConsoleModeVideo, String, String, sCRConsoleModeVideo, "Bios1")
    DefineGetterSetter(CRConsoleModeSound, String, String, sCRConsoleModeSound, "bios")

    DefineGetterSetter(DebugMenus, bool, Bool, sDebugMenus, false)

    /*
     * System
     */

    DefineSystemGetterSetterDeclaration(Emulator, String, String, sSystemEmulator)
    DefineSystemGetterSetterDeclaration(Core, String, String, sSystemCore)
    DefineSystemGetterSetterDeclaration(Ratio, String, String, sSystemRatio)
    DefineSystemGetterSetterDeclaration(Smooth, bool, Bool, sSystemSmooth)
    DefineSystemGetterSetterDeclaration(Rewind, bool, Bool, sSystemRewind)
    DefineSystemGetterSetterDeclaration(AutoSave, bool, Bool, sSystemAutoSave)
    DefineSystemGetterSetterDeclaration(Shaders, String, String, sSystemShaders)
    DefineSystemGetterSetterDeclaration(ShaderSet, String, String, sSystemShaderSet)
    DefineSystemGetterSetterDeclaration(Ignore, bool, Bool, sSystemIgnore)
    DefineSystemGetterSetterDeclaration(DemoInclude, bool, Bool, sSystemDemoInclude)
    DefineSystemGetterSetterDeclaration(DemoDuration, int, Int, sSystemDemoDuration)
    DefineSystemGetterSetterDeclaration(VideoMode, String, String, sSystemVideoMode)
    DefineSystemGetterSetterDeclaration(CrtHandheldFormat, String, String, sSystemCrtHandheldFormat)

    DefineEmulationStationSystemGetterSetterEnumDeclaration(GamelistDecoration, GamelistDecoration)
    DefineGetterSetterEnum(GlobalGamelistLayout, GamelistLayout, sGlobalGamelistLayout, GamelistLayout)
    DefineEmulationStationSystemGetterSetterEnumInheritedDeclaration(GamelistLayout, GamelistLayout)

    //! Global gamelist layout, resolved: having no global layout at all means the text list
    [[nodiscard]] GamelistLayout ResolvedGlobalGamelistLayout() const
    {
      GamelistLayout layout = GetGlobalGamelistLayout();
      return layout != GamelistLayout::Default ? layout : GamelistLayout::Legacy;
    }

    DefineEmulationStationSystemGetterSetterDeclaration(FilterAdult, bool, Bool, sSystemFilterAdult)
    DefineEmulationStationSystemGetterSetterDeclaration(FlatFolders, bool, Bool, sSystemFlatFolders)
    DefineEmulationStationSystemGetterSetterNumericEnumDeclaration(Sort, FileSorts::Sorts)
    DefineEmulationStationSystemGetterSetterNumericEnumDeclaration(RegionFilter, Regions::GameRegions)

    #undef DefineGetterSetter
    #undef DefineListGetterSetter
    #undef DefineGetterSetterParameterized

    /*
     * Direct Implementations - Pads
     */

    [[nodiscard]] String GetPad(int index) const { return AsString(String(sPadHeader).Append(index), ""); }
    RecalboxConf& SetPad(int index, const String& padid) { SetString(String(sPadHeader).Append(index), padid); return *this; }

    //! Physical port, in its own key: the pad id above is parsed on its first ':' and would swallow it
    [[nodiscard]] String GetPadPhysicalPath(int index) const { return AsString(String(sPadHeader).Append(index).Append(sPadPathSuffix), ""); }
    RecalboxConf& SetPadPhysicalPath(int index, const String& path) { SetString(String(sPadHeader).Append(index).Append(sPadPathSuffix), path); return *this; }

    /*
     * String-keyed dispatch (web manager bridge)
     */

    /*!
     * @brief Apply a global configuration value from its string key, going through the typed
     * setter so that watchers are notified. Used by the web manager bridge.
     * @param key Full configuration key (e.g. "emulationstation.showhidden")
     * @param value Value in string form
     * @return True if the key is known and was applied, false otherwise
     */
    bool SetByKey(const String& key, const String& value);

    /*!
     * @brief Apply a system-specific configuration value from its short key, going through the
     * typed setter so that watchers are notified. Used by the web manager bridge.
     * @param system Target system
     * @param shortKey Short key (e.g. "sort", "ignore")
     * @param value Value in string form
     * @return True if the key is known and was applied, false otherwise
     */
    bool SetSystemByKey(SystemData& system, const String& shortKey, const String& value);

    /*
     * System keys
     */

    static constexpr const char* sSystemEmulator             = "emulator";
    static constexpr const char* sSystemCore                 = "core";
    static constexpr const char* sSystemRatio                = "ratio";
    static constexpr const char* sSystemSmooth               = "smooth";
    static constexpr const char* sSystemRewind               = "rewind";
    static constexpr const char* sSystemAutoSave             = "autosave";
    static constexpr const char* sSystemIgnore               = "ignore";
    static constexpr const char* sSystemShaders              = "shaders";
    static constexpr const char* sSystemShaderSet            = "shaderset";
    static constexpr const char* sSystemFilterAdult          = "filteradultgames";
    static constexpr const char* sSystemRegionFilter         = "regionfilter";
    static constexpr const char* sSystemFlatFolders          = "flatfolders";
    static constexpr const char* sSystemSort                 = "sort";
    static constexpr const char* sSystemDemoInclude          = "demo.include";
    static constexpr const char* sSystemDemoDuration         = "demo.duration";
    static constexpr const char* sSystemVideoMode            = "videomode";
    static constexpr const char* sSystemCrtHandheldFormat    = "crthandheldformat";

    /*
     * Collection Keys
     */

    static constexpr const char* sCollectionHide             = "hide";
    static constexpr const char* sCollectionTheme            = "theme";
    static constexpr const char* sCollectionLimit            = "limit";

    /*
     * Key headers
     */

    static constexpr const char* sCollectionHeader           = "emulationstation.collection";
    static constexpr const char* sArcadeCollectionHeader     = "emulationstation.collection.arcade";

    /*
     * Keys
     */

    static constexpr const char* sExperimental               = "global.experimental";

    static constexpr const char* sGlobalGamelistLayout       = "global.gamelistlayout";
    static constexpr const char* sGlobalRatio                = "global.ratio";
    static constexpr const char* sGlobalSmooth               = "global.smooth";
    static constexpr const char* sGlobalRecalboxOverlays     = "global.recalboxoverlays";
    static constexpr const char* sGlobalRewind               = "global.rewind";
    static constexpr const char* sGlobalAutoSave             = "global.autosave";
    static constexpr const char* sGlobalSoftpatching         = "global.softpatching";
    static constexpr const char* sGlobalShaders              = "global.shaders";
    static constexpr const char* sGlobalShaderSet            = "global.shaderset";
    static constexpr const char* sGlobalQuitTwice            = "global.quitpresstwice";
    static constexpr const char* sGlobalHidePreinstalled     = "global.hidepreinstalledgames";
    static constexpr const char* sGlobalIntegerScale         = "global.integerscale";
    static constexpr const char* sGlobalShowFPS              = "global.showfps";
    static constexpr const char* sGlobalDemoDuration         = "global.demo.duration";
    static constexpr const char* sGlobalDemoInfoScreen       = "global.demo.infoscreenduration";
    static constexpr const char* sGlobalReduceLatency        = "global.reducelatency";
    static constexpr const char* sGlobalRunAhead             = "global.runahead";
    static constexpr const char* sGlobalHDMode               = "global.hdmode";
    static constexpr const char* sGlobalAutoBlitter          = "global.autoblitter";
    static constexpr const char* sGlobalWidescreen           = "global.widescreenmode";
    static constexpr const char* sGlobalShowSaveStateBeforeRun = "global.show.savestate.before.run";
    static constexpr const char* sSplashEnabled              = "system.splash.enabled";
    static constexpr const char* sDarkMode                   = "system.darkmode";
    static constexpr const char* sGlobalVulkanDriver         = "global.vulkandriver";
    static constexpr const char* sGlobalCrtHandheldFormat    = "global.crthandheldformat";

    static constexpr const char* sGlobalInputDriver          = "global.inputdriver";

    static constexpr const char* sHostname                   = "system.hostname";

    static constexpr const char* sWifiEnabled                = "wifi.enabled";
    static constexpr const char* sWifiConnect                = "wifi.connect";
    static constexpr const char* sWifiSSID                   = "wifi.ssid";
    static constexpr const char* sWifiKey                    = "wifi.key";

    static constexpr const char* sSwapValidateAndCancel      = "controllers.swapvalidateandcancel";

    static constexpr const char* sAudioVolume                = "audio.volume";
    static constexpr const char* sAudioMusicVolume           = "audio.music.volume";
    static constexpr const char* sAudioOptions               = "audio.mode";
    static constexpr const char* sAudioOuput                 = "audio.device";

    static constexpr const char* sMusicDisableRemote         = "music.remoteplaylist.enable";

    static constexpr const char* sScreenSaverTime            = "emulationstation.screensaver.time";
    static constexpr const char* sScreenSaverType            = "emulationstation.screensaver.type";
    static constexpr const char* sScreenSaverSystemList      = "global.demo.systemlist";

    static constexpr const char* sPopupHelp                  = "emulationstation.popoup.help";
    static constexpr const char* sPopupMusic                 = "emulationstation.popoup.music";
    static constexpr const char* sPopupNetplay               = "emulationstation.popoup.netplay";

    static constexpr const char* sThemeGeneric               = "emulationstation.theme.";
    static constexpr const char* sThemeCarousel              = "emulationstation.theme.carousel";
    static constexpr const char* sThemeTransition            = "emulationstation.theme.transition";
    static constexpr const char* sThemeRegion                = "emulationstation.theme.region";
    static constexpr const char* sThemeFolder                = "emulationstation.theme.folder";
    static constexpr const char* sGameTransition             = "emulationstation.game.transition";

    static constexpr const char* sCardReaderGeneric         = "cardreader.";

    static constexpr const char* sBrightness                 = "emulationstation.brightness";
    static constexpr const char* sClock                      = "emulationstation.clock";
    static constexpr const char* sShowHelp                   = "emulationstation.showhelp";
    static constexpr const char* sShowGameClipHelpItems      = "emulationstation.showgamecliphelpitems";
    static constexpr const char* sShowGameClipClippingItem   = "emulationstation.showgameclipclippingitem";
    static constexpr const char* sQuickSystemSelect          = "emulationstation.quicksystemselect";
    static constexpr const char* sListFastMoveEnabled        = "frontend.listfastmovenabled";
    static constexpr const char* sFilterAdultGames           = "emulationstation.filteradultgames";
    static constexpr const char* sEnableEditFavorites        = "frontend.enableeditfavorites";
    static constexpr const char* sFavoritesOnly              = "emulationstation.favoritesonly";
    static constexpr const char* sFavoritesFirst             = "emulationstation.favoritesfirst";
    static constexpr const char* sShowHidden                 = "emulationstation.showhidden";
    static constexpr const char* sShowOnlyLatestVersion      = "emulationstation.showonlylatestversion";
    static constexpr const char* sHideNoGames                = "emulationstation.hidenogames";
    static constexpr const char* sHideBoardGames             = "emulationstation.hideboardgames";
    static constexpr const char* sShowOnly3PlusPlayers       = "emulationstation.showonly3plusplayers";

    static constexpr const char* sSystemSorting              = "emulationstation.systemsorting";

    static constexpr const char* sBatteryHidden              = "emulationstation.battery.hidden";
    static constexpr const char* sPadOSD                     = "emulationstation.pads.osd";
    static constexpr const char* sPadOSDType                 = "emulationstation.pads.osd.type";
    static constexpr const char* sAutoPairOnBoot             = "controllers.bluetooth.autopaironboot";
    static constexpr const char* sControllersRumble          = "global.rumble";
    static constexpr const char* sGamepadProxyFilters        = "controllers.gamepadproxy.filters";

    static constexpr const char* sEsVideoMode                = "system.es.videomode";
    static constexpr const char* sGlobalVideoMode            = "global.videomode";
    static constexpr const char* sKodiVideoMode              = "kodi.videomode";
    static constexpr const char* sESForce43                  = "system.es.force43";

    static constexpr const char* sFirstTimeUse               = "system.firsttimeuse";
    static constexpr const char* sFirstTimeDual2             = "system.firsttimedual2";
    static constexpr const char* sFirstTimeJamma2            = "system.firsttimejamma2";
    static constexpr const char* sFirstTimeJvs               = "system.firsttimejvs";
    static constexpr const char* sFirstTimeCardReader        = "system.firsttimecardreader";
    static constexpr const char* sSystemLanguage             = "system.language";
    static constexpr const char* sSystemTimezone             = "system.timezone";
    static constexpr const char* sSystemKbLayout             = "system.kblayout";
    static constexpr const char* sVirtualKeyboardType        = "emulationstation.virtualkeyboard";
    static constexpr const char* sSystemManagerEnabled       = "system.manager.enabled";

    static constexpr const char* sOverclocking               = "system.overclocking";

    static constexpr const char* sPowerSwitch                = "system.power.switch";

    static constexpr const char* sKodiEnabled                = "kodi.enabled";
    static constexpr const char* sKodiAtStartup              = "kodi.atstartup";
    static constexpr const char* sKodiXButton                = "kodi.xbutton";

    static constexpr const char* sScraperSource              = "scraper.source";
    static constexpr const char* sScraperAuto                = "scraper.auto";
    static constexpr const char* sScraperGetNameFrom         = "scraper.getnamefrom";

    static constexpr const char* sRecalboxPrivateKey         = "patron.privatekey";

    static constexpr const char* sScreenScraperLogin         = "scraper.screenscraper.user";
    static constexpr const char* sScreenScraperPassword      = "scraper.screenscraper.password";
    static constexpr const char* sScreenScraperRegionPriority= "scraper.screenscraper.regionPriority";
    static constexpr const char* sScreenScraperRegion        = "scraper.screenscraper.region";
    static constexpr const char* sScreenScraperLanguage      = "scraper.screenscraper.language";
    static constexpr const char* sScreenScraperMainMedia     = "scraper.screenscraper.media";
    static constexpr const char* sScreenScraperBox           = "scraper.screenscraper.box";
    static constexpr const char* sScreenScraperVideo         = "scraper.screenscraper.video";
    static constexpr const char* sScreenScraperMarquee       = "scraper.screenscraper.marquee";
    static constexpr const char* sScreenScraperLogo          = "scraper.screenscraper.logo";
    static constexpr const char* sScreenScraperWantManual    = "scraper.screenscraper.manual";
    static constexpr const char* sScreenScraperWantMaps      = "scraper.screenscraper.maps";
    static constexpr const char* sScreenScraperWantTips       = "scraper.screenscraper.tips";
    static constexpr const char* sScreenScraperWantP2K       = "scraper.screenscraper.p2k";

    static constexpr const char* sNetplayEnabled             = "global.netplay.active";
    static constexpr const char* sNetplayLogin               = "global.netplay.nickname";
    static constexpr const char* sNetplayLobby               = "global.netplay.lobby";
    static constexpr const char* sNetplayPort                = "global.netplay.port";
    static constexpr const char* sNetplayRelay               = "global.netplay.relay";

    static constexpr const char* sRetroAchievementOnOff      = "global.retroachievements";
    static constexpr const char* sRetroAchievementHardcore   = "global.retroachievements.hardcore";
    static constexpr const char* sRetroAchievementLogin      = "global.retroachievements.username";
    static constexpr const char* sRetroAchievementPassword   = "global.retroachievements.password";
    static constexpr const char* sRetroAchievementSounds   = "global.retroachievements.sounds";

    static constexpr const char* sStartupGamelistOnly        = "emulationstation.gamelistonly";
    static constexpr const char* sStartupSelectedSystem      = "emulationstation.selectedsystem";
    static constexpr const char* sStartupStartOnGamelist     = "emulationstation.bootongamelist";
    static constexpr const char* sStartupHideSystemView      = "emulationstation.hidesystemview";
    static constexpr const char* sOneGameOneRom              = "emulationstation.onegameonerom";
    static constexpr const char* sRomPreferredRegion         = "emulationstation.rompreferredregion";
    static constexpr const char* sRomSecondPreferredRegion   = "emulationstation.romsecondpreferredregion";
    static constexpr const char* sRomPreferredFallback       = "emulationstation.rompreferredfallback";
    static constexpr const char* sDisplayByFileName          = "emulationstation.displaybyfilename";

    static constexpr const char* sMenuType                   = "emulationstation.menu";
    static constexpr const char* sHideSystemView             = "emulationstation.hidesystemview";
    static constexpr const char* sBootOnGamelist             = "emulationstation.bootongamelist";
    static constexpr const char* sForceBasicGamelistView     = "emulationstation.forcebasicgamelistview";

    static constexpr const char* sCollectionLastPlayed       = "emulationstation.collection.lastplayed";
    static constexpr const char* sCollectionMultiplayer      = "emulationstation.collection.multiplayer";
    static constexpr const char* sCollectionAllGames         = "emulationstation.collection.allgames";
    static constexpr const char* sCollectionLightGun         = "emulationstation.collection.lightgun";
    static constexpr const char* sCollectionPorts            = "emulationstation.collection.ports";
    static constexpr const char* sCollectionTate             = "emulationstation.collection.tate";
    static constexpr const char* sCollectionDial             = "emulationstation.collection.dial";
    static constexpr const char* sCollectionTrackball        = "emulationstation.collection.trackball";
    static constexpr const char* sCollectionGenre            = "emulationstation.collection.genre";
    static constexpr const char* sTateGameRotation           = "tate.gamerotation";
    static constexpr const char* sTateOnly                   = "emulationstation.tateonly";
    static constexpr const char* sYokoOnly                   = "emulationstation.yokoonly";

    static constexpr const char* sCollectionArcade           = "emulationstation.virtualarcade";
    static constexpr const char* sCollectionArcadeManufacturers = "emulationstation.virtualarcade.manufacturers";
    static constexpr const char* sCollectionArcadeHideOriginals = "emulationstation.virtualarcade.hideoriginals";
    static constexpr const char* sCollectionArcadePosition   = "emulationstation.virtualarcade.position";

    static constexpr const char* sArcadeViewEnhanced         = "emulationstation.arcade.view.enhanced";
    static constexpr const char* sArcadeViewFoldClones       = "emulationstation.arcade.view.hideclones";
    static constexpr const char* sArcadeViewHideBios         = "emulationstation.arcade.view.hidebios";
    static constexpr const char* sArcadeViewHideNonWorking   = "emulationstation.arcade.view.hidenonworking";
    static constexpr const char* sArcadeUseDatabaseNames     = "emulationstation.arcade.usedatabasenames";

    static constexpr const char* sBootOnGameEnabled             = "autorun.enabled";
    static constexpr const char* sBootOnGameSystemUUID          = "autorun.uuid";
    static constexpr const char* sBootOnGameGamePath            = "autorun.path";

    static constexpr const char* sUpdatesEnabled             = "updates.enabled";
    static constexpr const char* sUpdateType                 = "updates.type";

    static constexpr const char* sPadHeader                  = "emulationstation.pad";
    static constexpr const char* sPadPathSuffix              = ".path";

    static constexpr const char* sDebugLogs                  = "emulationstation.debuglogs";

    static constexpr const int sNetplayDefaultPort           = 55435;

    static constexpr const char* sSuperGameBoyOption         = "gb.supergameboy";

    static constexpr const char* sArcadeSystemHiddenManufacturers  = "hiddendrivers";

    static constexpr const char* sNetplayPasswords             = "netplay.password";
    static constexpr const char* sNetplayPasswordUseForPlayer  = "netplay.password.useforplayer";
    static constexpr const char* sNetplayPasswordUseForViewer  = "netplay.password.useforviewer";
    static constexpr const char* sNetplayPasswordLastForPlayer = "netplay.password.lastforplayer";
    static constexpr const char* sNetplayPasswordLastForViewer = "netplay.password.lastforviewer";
    static constexpr const char* sNetplayPasswordClient        = "netplay.password.client";
    static constexpr const char* sCRConsoleMode                = "cardreader.consolemode";
    static constexpr const char* sCRConsoleModeExit            = "cardreader.consolemodeexit";
    static constexpr const char* sCRConsoleModeVideo           = "cardreader.consolemodevideo";
    static constexpr const char* sCRConsoleModeSound           = "cardreader.consolemodesound";

    static constexpr const char* sDebugMenus                   = "es.menu.debug";


    /*
     * Public enumeration getter/setter
     */

    static UpdateType UpdateTypeFromString(const String& pad);
    static const String& UpdateTypeFromEnum(UpdateType type);

    static RecalboxConf::Transition TransitionFromString(const String& transition);
    static const String& TransitionFromEnum(RecalboxConf::Transition transition);

    // Public so the web-manager system dispatch (free functions in the .cpp) can convert strings to enums.
    static GamelistDecoration GamelistDecorationFromString(const String& gameDecoration);
    static GamelistLayout GamelistLayoutFromString(const String& ut);

  private:
    HashMap<String, Array<IRecalboxConfChanged*>> mWatchers;

    /*
     * Culture
     */

    static String GetLanguage();
    static String GetCountry();

    /*
     * Enumeration getter/setter
     */

    static SoftPatching SoftPatchingFromString(const String& menu);
    static const String& SoftPatchingFromEnum(SoftPatching screensaver);
    static Screensaver ScreensaverFromString(const String& menu);
    static const String& ScreensaverFromEnum(Screensaver screensaver);
    static Menu MenuFromString(const String& menu);
    static const String& MenuFromEnum(Menu menu);
    static Relay RelayFromString(const String& relay);
    static const String& RelayFromEnum(Relay relay);
    static SystemSorting SystemSortingFromString(const String& systemSorting);
    static const String& SystemSortingFromEnum(SystemSorting systemSorting);
    static ScraperType ScraperTypeFromString(const String& menu);
    static const String& ScraperTypeFromEnum(ScraperType type);
    static PadOSDType PadOSDTypeFromString(const String& pad);
    static const String& PadOSDTypeFromEnum(PadOSDType type);
    static String GamelistDecorationFromEnum(GamelistDecoration gameDecoration);
    static const String& GamelistLayoutFromEnum(GamelistLayout type);
    static VirtualKeyboardType VirtualKeyboardTypeFromString(const String& kind);
    static const String& VirtualKeyboardTypeFromEnum(VirtualKeyboardType kind);

    /*!
     * @brief Convert string audiomode to AudioMode enum
     * @param audioMode Audio mode as string
     * @return Audio mode as enum
     */
    static AudioMode AudioModeFromString(const String& audioMode);
    /*!
     * @brief Convert AudioMode enum to string
     * @param audioMode Audio mode as enum
     * @return Audio mode as string
     */
    static const String& AudioModeFromEnum(AudioMode audioMode);

};

DEFINE_BITFLAG_ENUM(RecalboxConf::GamelistDecoration, int)
