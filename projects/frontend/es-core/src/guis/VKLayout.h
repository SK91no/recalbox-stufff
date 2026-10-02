//
// Created for the classic on-screen virtual keyboard.
//
#pragma once

#include <utils/String.h>

//! What a key on the classic virtual keyboard does
enum class VKKeyKind : unsigned char
{
  Char,       //!< Emits a character (chosen between normal / shifted / altgr)
  Backspace,  //!< Delete char left of cursor
  Enter,      //!< Validate & close
  Space,      //!< Emits ' '
  Shift,      //!< Shift modifier (short tap = once, long press = lock)
  Ctrl,       //!< Ctrl modifier (visual only, follows same lock pattern)
  Alt,        //!< Alt modifier (visual only, follows same lock pattern)
  AltGr,      //!< AltGr modifier (gives access to 3rd character layer)
  Left,       //!< Move cursor left in edited text
  Right,      //!< Move cursor right in edited text
};

//! A single key in the on-screen layout
struct VKKey
{
  VKKeyKind kind;             //!< Behaviour
  unsigned int normal;        //!< Codepoint emitted with no modifier (0 for non-Char)
  unsigned int shifted;       //!< Codepoint emitted with Shift (0 → falls back to normal)
  unsigned int altgr;         //!< Codepoint emitted with AltGr (0 → no AltGr variant)
  unsigned short widthx100;   //!< Key width in hundredths of unit-cell (100 = 1.0)
  const char* label;          //!< Label for non-Char keys (nullptr for Char)
};

//! A row of keys
struct VKRow
{
  const VKKey* keys;  //!< First key
  int count;          //!< Number of keys
};

//! A full keyboard layout
struct VKLayout
{
  const char* code;       //!< xkb-style code ("us", "fr", "de", "dk", "es", "it")
  const VKRow* rows;      //!< First row
  int rowCount;           //!< Number of rows
  int unitsPerRowX100;    //!< Total width units of the widest row, x100
};

class VKLayouts
{
  public:
    //! Resolve a layout code to its definition (falls back to "us" if unknown)
    static const VKLayout& Get(const String& code);
};
