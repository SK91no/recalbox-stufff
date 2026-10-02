//
// Created by Pit64 on 11/12/2025.
//
#include "KeyboardLayoutMapper.h"
#include <utils/Log.h>

KeyboardLayoutMapper::KeyboardLayoutMapper(const String& layout)
  : mLayout(layout)
{
  InitializeMappings();
  { LOG(LogInfo) << "[KeyboardLayoutMapper] Initialized layout: " << mLayout; }
}

SDL_Keycode KeyboardLayoutMapper::MapScancode(SDL_Scancode const scancode) const
{
  const SDL_Keycode* mapped = mScancodeMap.try_get(scancode);
  if (mapped != nullptr)
    return *mapped;

  return SDL_GetKeyFromScancode(scancode);
}

String KeyboardLayoutMapper::RemapText(const char* text, const bool altGrPressed) const
{
  if (text == nullptr || text[0] == '\0')
    return String();

  if (mCharMap.empty() && mAltGrCharMap.empty())
    return String(text);

  String result;
  const unsigned char* p = reinterpret_cast<const unsigned char*>(text);

  while (*p != '\0')
  {
    unsigned int codepoint;
    int len = 1;

    // Decode UTF-8 character
    if (*p < 0x80)
    {
      codepoint = *p;
    }
    else if ((*p & 0xE0) == 0xC0)
    {
      codepoint = ((*p & 0x1F) << 6) | (p[1] & 0x3F);
      len = 2;
    }
    else if ((*p & 0xF0) == 0xE0)
    {
      codepoint = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
      len = 3;
    }
    else if ((*p & 0xF8) == 0xF0)
    {
      codepoint = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
      len = 4;
    }
    else
    {
      // Invalid UTF-8, skip
      p++;
      continue;
    }

    // Try to map the codepoint
    if (altGrPressed)
    {
      const char* const* altGrMapped = mAltGrCharMap.try_get(codepoint);
      if (altGrMapped != nullptr)
      {
        result.Append(*altGrMapped);
        p += len;
        continue;
      }
    }

    const char* const* mapped = mCharMap.try_get(codepoint);
    if (mapped != nullptr)
    {
      result.Append(*mapped);
    }
    else
    {
      // No mapping, copy original UTF-8 sequence
      result.Append(reinterpret_cast<const char*>(p), len);
    }

    p += len;
  }

  return result;
}

bool KeyboardLayoutMapper::IsUsingNativeSDL() const
{
  // Lazy detection of video driver to ensure it's initialized
  const char* videoDriver = SDL_GetCurrentVideoDriver();
  if (videoDriver != nullptr)
  {
    if (strcmp(videoDriver, "x11") == 0 || strcmp(videoDriver, "wayland") == 0)
    {
      return true;
    }
  }
  return false;
}

void KeyboardLayoutMapper::InitializeMappings()
{
  mScancodeMap.clear();
  mCharMap.clear();
  mAltGrCharMap.clear();

  // Always initialize layout maps for non-US layouts (kmsdrm needs them)
  if (mLayout == "fr" || mLayout == "fr_FR")
    AddFrenchLayout();
  else if (mLayout == "de" || mLayout == "de_DE")
    AddGermanLayout();
  else if (mLayout == "es" || mLayout == "es_ES")
    AddSpanishLayout();
  else if (mLayout == "it" || mLayout == "it_IT")
    AddItalianLayout();
  else if (mLayout == "dk" || mLayout == "dk_DK")
    AddDanishLayout();
  else if (mLayout == "uk" || mLayout == "en_GB")
    AddUKLayout();
}

void KeyboardLayoutMapper::AddFrenchLayout()
{
  static const HashMap<unsigned int, const char*> frenchCharMap = {
    {'a', "q"}, {'q', "a"}, {'z', "w"}, {'w', "z"}, {'A', "Q"},
    {'Q', "A"}, {'Z', "W"}, {'W', "Z"}, {'`', "²"}, {'1', "&"},
    {'2', "é"}, {'3', "\""}, {'4', "'"}, {'5', "("}, {'6', "-"},
    {'7', "è"}, {'8', "_"}, {'9', "ç"}, {'0', "à"}, {'-', ")"},
    {'[', "^"}, {']', "$"}, {';', "m"}, {'\'', "ù"}, {'\\', "*"},
    {'m', ","}, {',', ";"}, {'.', ":"}, {'/', "!"}, {'!', "1"},
    {'@', "2"}, {'#', "3"}, {'$', "4"}, {'%', "5"}, {'^', "6"},
    {'&', "7"}, {'*', "8"}, {'(', "9"}, {')', "0"}, {'_', "°"},
    {'{', "¨"}, {'}', "£"}, {':', "%"}, {'|', "μ"}, {'M', "?"},
    {'<', "."}, {'>', "/"}, {'?', "§"}
  };

  static const HashMap<unsigned int, const char*> frenchAltGrMap = {
    {'2', "~"}, {'3', "#"}, {'4', "{"}, {'5', "["}, {'6', "|"},
    {'7', "`"}, {'8', "\\"}, {'9', "^"}, {'0', "@"}, {')', "]"},
    {'=', "}"}, {'e', "€"}
  };

  mCharMap = frenchCharMap;
  mAltGrCharMap = frenchAltGrMap;
}

void KeyboardLayoutMapper::AddGermanLayout()
{
  static const HashMap<unsigned int, const char*> germanCharMap = {
    {'z', "y"}, {'y', "z"}, {'Z', "Y"}, {'Y', "Z"}, {'-', "ß"},
    {'_', "?"}, {'[', "ü"}, {'{', "Ü"},{';', "ö"}, {':', "Ö"},
    {'\'', "ä"}, {'"', "Ä"},{'`', "^"}, {'~', "°"}, {'=', "´"},
    {'+', "`"}
  };

  static const HashMap<unsigned int, const char*> germanAltGrMap = {
    {'q', "@"}, {'e', "€"}, {'7', "{"}, {'8', "["},{'9', "]"},
    {'0', "}"}, {'-', "\\"}, {'=', "~"},{'<', "|"}, {'+', "~"}
  };

  mCharMap = germanCharMap;
  mAltGrCharMap = germanAltGrMap;
}

void KeyboardLayoutMapper::AddSpanishLayout()
{
  static const HashMap<unsigned int, const char*> spanishCharMap = {
    {'`', "º"}, {'-', "'"}, {'=', "¡"}, {'[', "`"}, {']', "+"},
    {'/', "-"}, {0x00AA, "~"}, {'@', "\""},  {'#', "."},
    {'^', "&"}, {'&', "/"}, {'*', "("}, {'(', ")"}, {')', "="},
    {'_', "?"}, {'+', "¿"}, {'{', "^"}, {'}', "*"}, {':', "¨"},
    {'"', "ç"}, {'<', ";"}, {'>', ":"}, {'?', "_"}
  };

  static const HashMap<unsigned int, const char*> spanishAltGrMap = {
    {'`', "\\"}, {'1', "|"}, {'2', "@"}, {'3', "#"}, {'4', "~"},
    {'6', "¬"}, {'e', "€"}, {';', "{"}, {'\'', "}"}
  };

  mCharMap = spanishCharMap;
  mAltGrCharMap = spanishAltGrMap;
}

void KeyboardLayoutMapper::AddItalianLayout()
{
  static const HashMap<unsigned int, const char*> italianCharMap = {
    {'`', "\\"}, {'-', "'"}, {'=', "ì"}, {'~', "|"}, {'_', "?"},
    {'+', "^"}, {'@', "\""}, {'#', "£"}, {'^', "&"}, {'&', "/"},
    {'*', "("}, {'(', ")"}, {')', "="}, {'[', "è"}, {']', "+"},
    {'{', "é"}, {'}', "*"}, {';', "ò"}, {':', "ç"}, {'\'', "à"},
    {'"', "°"}, {'\\', "ù"}, {'|', "§"}, {'/', "-"}, {'?', "_"}
  };

  static const HashMap<unsigned int, const char*> italianAltGrCharMap = {
    {'[', "["}, {']', "]"}, {';', "@"}, {'\'', "#"},
    {'\\', "{"}, {'=', "}"}, {'e', "€"}
  };

  mCharMap = italianCharMap;
  mAltGrCharMap = italianAltGrCharMap;
}

void KeyboardLayoutMapper::AddDanishLayout()
{
  static const HashMap<unsigned int, const char*> danishCharMap = {
    {'-', "+"}, {'=', "´"},{'_', "?"}, {'+', "`"},{'[', "å"},
    {']', "¨"}, {'{', "Å"}, {'}', "^"},{';', "æ"}, {':', "Æ"},
    {'\'', "ø"}, {'"', "Ø"},{'\\', "'"}, {'|', "*"},{'/', "-"},
    {'?', "_"}
  };

  static const HashMap<unsigned int, const char*> danishAltGrMap = {
    {'2', "@"}, {'3', "£"}, {'4', "$"}, {'5', "€"},{'7', "{"},
    {'8', "["}, {'9', "]"}, {'0', "}"},{'[', "\\"}, {']', "~"},
    {'e', "€"}
  };

  mCharMap = danishCharMap;
  mAltGrCharMap = danishAltGrMap;
}

void KeyboardLayoutMapper::AddUKLayout()
{
  static const HashMap<unsigned int, const char*> UKCharMap = {
    {'~', "¬"}, {'@', "\""}, {'#', "£"}, {'"', "@"}
  };

  static const HashMap<unsigned int, const char*> UKAltGrCharMap = {};

  mCharMap = UKCharMap;
  mAltGrCharMap = UKAltGrCharMap;
}