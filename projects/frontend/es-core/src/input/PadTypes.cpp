//
// Created by bkg2k on 08/05/2026.
//

#include <input/PadTypes.h>
#include <utils/String.h>

const String& PadTypes::ToString(Type type)
{
  switch (type)
  {
    case XBox:        { static String s("xbox"); return s; }
    case Playstation: { static String s("sony"); return s; }
    case Arcade8:     { static String s("arcade8"); return s; }
    case Arcade6:     { static String s("arcade6"); return s; }
    case Arcade4:     { static String s("arcade4"); return s; }
    case Gamecube:    { static String s("gamecube"); return s; }
    case Megadrive6:  { static String s("megadrive6"); return s; }
    case Megadrive3:  { static String s("megadrive3"); return s; }
    case n64:         { static String s("n64"); return s; }
    case HeightBitdo:
    case __Count:
    default:
      break;
  }
  static String s("8bitdo"); return s;
}

PadTypes::Type PadTypes::FromString(const String& type)
{
  if (type == "xbox")       return XBox;
  if (type == "sony")       return Playstation;
  if (type == "arcade8")    return Arcade8;
  if (type == "arcade6")    return Arcade6;
  if (type == "arcade4")    return Arcade4;
  if (type == "gamecube")   return Gamecube;
  if (type == "megadrive6") return Megadrive6;
  if (type == "megadrive3") return Megadrive3;
  if (type == "n64")        return n64;
  return HeightBitdo;
}

PadTypes::Type PadTypes::FromControllerName(const String& deviceName)
{
  static const struct { const char* needle; Type type; } sPadNames[] =
  {
    { "8bitdo",      HeightBitdo },
    { "xbox",        XBox },
    { "x-box",       XBox },
    { "stadia",      XBox },
    { "sony",        Playstation },
    { "playstation", Playstation },
    { "jamma",       Arcade6 },        // JammaControllerP*
    { "recalboxjvs", Arcade6 },        // RecalboxJVS
  };
  String name = deviceName.ToLowerCaseUTF8();
  for (const auto& pad : sPadNames)
    if (name.Contains(pad.needle)) return pad.type;
  return __Count;
}
