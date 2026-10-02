//
// Created by digitalLumberjack on 2026-06-10.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
// Semantic facade over the JVS firmware sysfs (/sys/devices/platform/recalboxjvs/*).
// Outside callers must NOT hard-code sysfs attribute names or compare raw strings —
// go through this class. Add a new accessor here when you need a new field.
//
#pragma once

#include <hardware/JvsSysfs.h>
#include <utils/String.h>
#include <utils/Log.h>
#include <cstdlib>

class JvsCab
{
  public:
    // ── Cab type (raw string from sysfs; menu list lives in MenuDataProvider) ─
    static String      CurrentValue()         { return JvsSysfs::ReadString("cab_type"); }
    static void        SetType(const char* v) { JvsSysfs::WriteString("cab_type", v); }
    static const char* DefaultTypeValue()     { return sNotConfigured; }

    //! Raw sysfs values (use these to build menu entries — keep menu in sync with kernel).
    static constexpr const char* sNotConfigured       = "not_configured";
    static constexpr const char* sJoystick            = "joystick";
    static constexpr const char* sRace                = "race";
    static constexpr const char* sGun                 = "gun";
    static constexpr const char* sCrazyTaxi           = "crazy_taxi";
    static constexpr const char* sEighteenWheeler     = "18_wheeler";
    static constexpr const char* sJamboSafari         = "jambo_safari";
    static constexpr const char* sHouseOfTheDead3     = "house_of_the_dead_3";
    static constexpr const char* sLupin3              = "lupin_3";
    static constexpr const char* sNewNetCity          = "new_net_city";
    static constexpr const char* sNaomiNUC            = "naomi_nuc";
    static constexpr const char* sVewlix              = "vewlix";
    static constexpr const char* sConfidentialMission = "confidential_mission";
    static constexpr const char* sGunCustom           = "gun_custom";
    static constexpr const char* sJoyBlast            = "blast_city";
    static constexpr const char* sNetCity             = "net_city";

    // ── Capabilities derived from cab type (firmware-sourced via sysfs) ──
    //! Cab family read from kernel sysfs (firmware-authoritative).
    static String Family() { return JvsSysfs::ReadString("cab_family"); }

    //! True iff cab family is joystick (treat NotConfigured as joystick-like).
    static bool IsJoystickFamily() { const String f = Family(); return f.empty() || f == "none" || f == "joystick"; }
    static bool IsGunFamily()      { return Family() == "gun"; }
    static bool IsRaceFamily()     { return Family() == "race"; }

    //! User can choose how many controllers are wired on the cab panel (1 vs 2).
    static bool SupportsPlayersOnPanel() { return IsJoystickFamily() || IsGunFamily(); }
    //! Cab has a JAMMA-style button panel whose button count is user-selectable.
    static bool SupportsPanelType() { return IsJoystickFamily(); }
    //! Cab can host two linked IO boards in 4-player mode (1vs1 / 2vs2 split).
    static bool SupportsLinkMode() { return IsJoystickFamily(); }
    //! Lightgun cab (X/Y flips relevant). Covers generic + every specific gun cab.
    static bool IsGunCab() { return IsGunFamily(); }

    //! Hard chassis screentype lock (firmware-sourced via sysfs).
    static String ScreenLock()  { return JvsSysfs::ReadString("cab_screen_lock"); }
    static bool   Is15kHzOnly() { return ScreenLock() == "khz15"; }
    static bool   Is31kHzOnly() { return ScreenLock() == "khz31"; }
    //! Soft default screentype suggested by the cab (firmware-sourced).
    static String DefaultScreentype() { return JvsSysfs::ReadString("cab_default_screentype"); }

    // ── Hardware DIPs (latched at firmware boot, polled at runtime) ──────
    static bool IsHdmiOnly()  { return JvsSysfs::ReadBool("hdmi_only"); }
    static bool IsKhz15Only() { return JvsSysfs::ReadBool("khz15_only"); }

    //! True when the firmware UI cannot be shown (DIP HDMI-only or 15kHz-only).
    //! In that case ES becomes the only place to configure the cab.
    static bool FirmwareUiDisabled() { return IsHdmiOnly() || IsKhz15Only(); }

    // ── Players on panel ──────────────────────────────────────────────────
    static String      CurrentPlayersOnPanel()           { return JvsSysfs::ReadString("players_on_panel"); }
    static void        SetPlayersOnPanel(const char* v)  { JvsSysfs::WriteString("players_on_panel", v); }
    static const char* DefaultPlayersOnPanelValue()      { return "2"; }

    // ── Link mode (1vs1 / 2vs2) ───────────────────────────────────────────
    static String      CurrentLinkMode()                 { return JvsSysfs::ReadString("link_mode"); }
    static void        SetLinkMode(const char* v)        { JvsSysfs::WriteString("link_mode", v); }
    static const char* DefaultLinkModeValue()            { return "2vs2"; }

    static constexpr const char* sLinkOneVsOne = "1vs1";
    static constexpr const char* sLinkTwoVsTwo = "2vs2";

    // ── Toggles ───────────────────────────────────────────────────────────
    static bool StartQuickExit()           { return JvsSysfs::ReadBool("start_quick_exit"); }
    static void SetStartQuickExit(bool v)  { JvsSysfs::WriteBool("start_quick_exit", v); }

    static bool SkipBootLogo()             { return JvsSysfs::ReadBool("skip_boot_logo"); }
    static void SetSkipBootLogo(bool v)    { JvsSysfs::WriteBool("skip_boot_logo", v); }

    static bool FlipGunX()                 { return JvsSysfs::ReadBool("flip_gun_x"); }
    static void SetFlipGunX(bool v)        { JvsSysfs::WriteBool("flip_gun_x", v); }

    static bool FlipGunY()                 { return JvsSysfs::ReadBool("flip_gun_y"); }
    static void SetFlipGunY(bool v)        { JvsSysfs::WriteBool("flip_gun_y", v); }

    // ── JVS outputs forwarder (recalboxjvs-outputsd) ───────────────────────
    // Which cores emit outputs we can forward (Flycast + any libretro-mame).
    static bool CoreCanDriveOutputs(const String& core) { return core == "flycast-next" || core.StartsWith("mame"); }

    static void ArmOutputs(const String& romName, const String& core)
    {
        if (!JvsSysfs::Available() || !CoreCanDriveOutputs(core)) return;
        String cmd("/usr/bin/recalboxjvs-outputsd arm ");
        cmd.Append(romName).Append(' ').Append(core).Append(" 2>/dev/null");
        (void)std::system(cmd.c_str());
    }
    static void DisarmOutputs()
    {
        if (!JvsSysfs::Available()) return;
        (void)std::system("/usr/bin/recalboxjvs-outputsd disarm 2>/dev/null");
    }

    // ── Board identity (firmware-sourced via sysfs, read-only) ─────────────
    static bool   IsBound()          { return JvsSysfs::Available(); }
    static String FirmwareVersion()  { return JvsSysfs::ReadString("firmware_version"); }
    static int    FirmwareMajor()    { return JvsSysfs::ReadInt("firmware_major", -1); }
    static int    FirmwareMinor()    { return JvsSysfs::ReadInt("firmware_minor", -1); }
    static int    HardwareRevision() { return JvsSysfs::ReadInt("hw_revision", -1); }
    static int    IoBoardCount()     { return JvsSysfs::ReadInt("io_board_count", 0); }
    static bool   AnalogEnabled()    { return JvsSysfs::ReadBool("analog_enabled"); }

    static void LogDetection()
    {
        if (!IsBound()) return;
        auto yn  = [](bool b) { return b ? "yes" : "no"; };
        auto str = [](const String& s) { return s.empty() ? String("n/a") : s; };
        { LOG(LogInfo) << "[JVS] Detected JVS board: firmware(" << str(FirmwareVersion())
                       << "), major(" << FirmwareMajor()
                       << "), minor(" << FirmwareMinor()
                       << "), hw_rev(" << HardwareRevision()
                       << "), io_boards(" << IoBoardCount()
                       << "), cab_type(" << str(CurrentValue())
                       << "), cab_family(" << str(Family())
                       << "), analog(" << yn(AnalogEnabled())
                       << "), screen_lock(" << str(ScreenLock())
                       << "), screen_default(" << str(DefaultScreentype())
                       << "), hdmi_only(" << yn(IsHdmiOnly())
                       << "), khz15_only(" << yn(IsKhz15Only())
                       << "), players(" << str(CurrentPlayersOnPanel())
                       << "), link_mode(" << str(CurrentLinkMode())
                       << "), start_quick_exit(" << yn(StartQuickExit())
                       << "), skip_boot_logo(" << yn(SkipBootLogo())
                       << "), flip_gun_x(" << yn(FlipGunX())
                       << "), flip_gun_y(" << yn(FlipGunY()) << ")"; }
    }
};
