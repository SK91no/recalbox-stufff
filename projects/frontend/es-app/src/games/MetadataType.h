//
// Created by bkg2k on 21/06/23.
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

enum class MetadataType : unsigned long long int
{
  // Zero'ed
  None               = 0x000000000, //!< Zero'ed bitflag
  // Path - Not editable
  Path               = 0x000000001, //!< Path
  // Texts
  Name               = 0x000000002, //!< Game name
  Alias              = 0x000000004, //!< Aliases
  License            = 0x000000008, //!< Licenses
  Synopsis           = 0x000000010, //!< Synopsis text
  Publisher          = 0x000000020, //!< Publisher
  Developer          = 0x000000040, //!< Developer
  Players            = 0x000000080, //!< Players
  ReleaseDate        = 0x000000100, //!< Epoch of release date
  P2K                = 0x000000200, //!< P2k file & path
  Crc32              = 0x000000400, //!< CRC32
  Region             = 0x000000800, //!< Game regions (comma separated)
  Rating             = 0x000001000, //!< Rating (float 0.0 - 1.0)
  Genre              = 0x000002000, //!< Text genre
  GenreId            = 0x000004000, //!< Normalized genres
  // Media
  Image              = 0x000008000, //!< Main image
  Box                = 0x000010000, //!< Box image (2D/3D)
  Video              = 0x000020000, //!< Video snap
  TipsAndTricks      = 0x000040000, //!< Video snap
  Manual             = 0x000080000, //!< Manuel pdf/txt
  Map                = 0x000100000, //!< Game maps (images)
  Marquee            = 0x000200000, //!< Marquee
  Logo               = 0x000400000, //!< Logo
  LastPatch          = 0x000800000, //!< Last selected patch
  // Extra data
  LastPlayed         = 0x001000000, //!< Last played date
  TimePlayed         = 0x002000000, //!< Time played in seconds
  PlayCount          = 0x004000000, //!< Play counter
  Favorite           = 0x008000000, //!< Game is favorite
  Rotation           = 0x010000000, //!< Rotation data for TATE or special games
  Hidden             = 0x020000000, //!< Game is hidden
  Adult              = 0x040000000, //!< Adult state
  // Hooked data
  Emulator           = 0x100000000, //!< Emulator
  Core               = 0x200000000, //!< Core
  Ratio              = 0x400000000, //!< Screen ratio
};

DEFINE_BITFLAG_ENUM(MetadataType, unsigned long long int)
