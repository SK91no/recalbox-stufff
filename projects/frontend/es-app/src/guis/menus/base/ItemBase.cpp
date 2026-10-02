//
// Created by bkg2k on 21/08/24.
//

#include <guis/menus/base/ItemBase.h>
#include <guis/menus/base/IMenuInterface.h>
#include <systems/SystemData.h>
#include "emulators/EmulatorManager.h"
#include "usernotifications/ScriptAttributes.h"
#include "usernotifications/ScriptDescriptor.h"

bool ItemBase::CalibrateLeftAndRightParts(int leftWidth, int& rightDynamicWidth, int rightStaticWidth)
{
  // Adjust width
  int half = mDataProvider.ItemWidth() / 2;
  if (rightDynamicWidth + rightStaticWidth + leftWidth > mDataProvider.ItemWidth())
  {
    // Reduce right option to make room for the label
    if (leftWidth < half) { rightDynamicWidth = half - rightStaticWidth; return true; }
    // Keep option space and let the label scroll by itself
    else if (rightDynamicWidth + rightStaticWidth < half) return false;
    // Both are bigger than half the space. Reduce the right part to the right half and let the label scroll in the left half
    else { rightDynamicWidth = half - rightStaticWidth; return true; }
  }
  else
  {
    // Clamp dynamic part to half the total item width
    if (rightDynamicWidth + rightStaticWidth > half) { rightDynamicWidth = half - rightStaticWidth; return true; }
  }
  return false;
}


void ItemBase::ReplaceParameters(InheritableContext from, String& to)
{
  static String systemName("{SYSTEM.NAME}");
  static String systemDefaultEmulator("{SYSTEM.DEFAULTEMULATOR}");
  static String gameName("{GAME.NAME}");
  static String scriptName("{SCRIPT.NAME}");
  static String deviceName("{DEVICE.NAME}");
  static String index("{INDEX}");
  static String iconWarning("{ICON.WARNING}");
  static String themeName("{THEME.NAME}");
  static String optionName("{THEME.OPTION.NAME}");
  static String optionHelp("{THEME.OPTION.HELP}");
  static String unknownSystem("<No system available>");
  static String unknownGame("<No game available>");
  static String unknownScript("<No script available>");
  static String unknownDevice("<No device available>");
  static String unknownOption("<No theme option available>");

  to.Replace(systemName, from.HasSystem() ? from.System()->Descriptor().FullName() : unknownSystem)
    .Replace(gameName, from.HasGame() ? from.Game()->Name() : unknownGame)
    .Replace(deviceName, from.HasDevice() ? from.Device()->DisplayableName() : (from.HasPhysicalDevice() ? from.PhysicalDevice()->DisplayableName() : unknownDevice))
    .Replace(optionName, from.HasOption() ? from.Option()->DisplayName() : unknownOption)
    .Replace(optionHelp, from.HasOption() ? from.Option()->Help() : unknownOption)
    .Replace(themeName, ThemeManager::Instance().CurrentThemeName())
    .Replace(iconWarning, "\uf1ca")
    .Replace(index, String(from.Index()));
  if (to.Contains(scriptName))
  {
    if (from.HasScript())
    {
      const ScriptDescriptor& script = *from.Script();
      String name = script.mPath.FilenameWithoutDecorations();
      if ((script.mAttribute & (ScriptAttributes::Synced | ScriptAttributes::ReportProgress)) != 0) name.Append(' ');
      if (hasFlag(script.mAttribute, ScriptAttributes::Synced)) name.AppendUTF8(0xF1AF);
      if (hasFlag(script.mAttribute, ScriptAttributes::ReportProgress)) name.AppendUTF8(0xF1AE);
      to.Replace(scriptName, name);
    }
    else to.Replace(scriptName, unknownScript);
  }
  if (to.Contains(systemDefaultEmulator))
  {
    if (from.HasSystem())
    {
      String emulator;
      String core;
      EmulatorManager::GetSystemEmulator(*from.System(), emulator, core);
      to.Replace(systemDefaultEmulator, emulator == core ? core : emulator.Append(' ').Append(core));
    }
    else to.Replace(systemDefaultEmulator, unknownSystem);
  }
}

ItemBase& ItemBase::ReplaceParameters(String& to)
{
  ReplaceParameters(Context(), to);
  return *this;
}

ItemBase& ItemBase::ReplaceParameters()
{
  ReplaceParameters(mLabel);
  ReplaceParameters(mHelp);
  ReplaceParameters(mHelpUnselectable);
  mLabel.UpperCaseUTF8();
  //mHelp.UpperCaseUTF8();
  //mHelpUnselectable.UpperCaseUTF8();
  return *this;
}

ItemBase& ItemBase::ReplaceParameters(const String& parameter1, bool uppercase)
{
  mLabel = (_F(mLabel) / parameter1)();
  mHelp = (_F(mHelp) / parameter1)();
  mHelpUnselectable = (_F(mHelpUnselectable) / parameter1)();
  if (uppercase)
  {
    mLabel.UpperCaseUTF8();
    //mHelp.UpperCaseUTF8();
    //mHelpUnselectable.UpperCaseUTF8();
  }
  return *this;
}

ItemBase& ItemBase::ReplaceParameters(const String& parameter1, const String& parameter2, bool uppercase)
{
  mLabel = (_F(mLabel) / parameter1 / parameter2)();
  mHelp = (_F(mHelp) / parameter1 / parameter2)();
  mHelpUnselectable = (_F(mHelpUnselectable) / parameter1 / parameter2)();
  if (uppercase)
  {
    mLabel.UpperCaseUTF8();
    //mHelp.UpperCaseUTF8();
    //mHelpUnselectable.UpperCaseUTF8();
  }
  return *this;
}

ItemBase& ItemBase::ReplaceParameters(const String& parameter1, const String& parameter2, const String& parameter3, bool uppercase)
{
  mLabel = (_F(mLabel) / parameter1 / parameter2 / parameter3)();
  mHelp = (_F(mHelp) / parameter1 / parameter2 / parameter3)();
  mHelpUnselectable = (_F(mHelpUnselectable) / parameter1 / parameter2 / parameter3)();
  if (uppercase)
  {
    mLabel.UpperCaseUTF8();
    //mHelp.UpperCaseUTF8();
    //mHelpUnselectable.UpperCaseUTF8();
  }
  return *this;
}
