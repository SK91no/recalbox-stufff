#pragma once

#include <utils/cplusplus/Bitflags.h>

//! Item type
enum class ItemType : char
{
  Game,
  Folder,
  Root,
  Empty,
  Header,
};

enum class ThemeItemType : char
{
  None   = 0, //!< No type
  Game   = 1, //!< Game
  Folder = 2, //!< Folder (any type of folder)
  Header = 4, //!< Header
};

DEFINE_BITFLAG_ENUM(ThemeItemType, char)
