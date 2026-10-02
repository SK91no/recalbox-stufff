//
// Controller database
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "ControllerDatabase.h"
#include <utils/Files.h>
#include <utils/os/fs/Path.h>

// ---- ControlType <-> string ----

namespace
{
  //! Exhaustive base-type table (see controller.lst). Lookup is exact-match,
  //! so ordering is irrelevant. Used only at load time.
  struct ControlTypeName { const char* Name; ControlType Type; };
  const ControlTypeName sControlTypeNames[] =
  {
    { "joy",               ControlType::Joy               },
    { "joyvertical",       ControlType::JoyVertical       },
    { "mahjong",           ControlType::Mahjong           },
    { "stick",             ControlType::Stick             },
    { "mouse",             ControlType::Mouse             },
    { "keypad",            ControlType::Keypad            },
    { "only_buttons",      ControlType::OnlyButtons       },
    { "keyboard",          ControlType::Keyboard          },
    { "dial",              ControlType::Dial              },
    { "paddle",            ControlType::Paddle            },
    { "pedal",             ControlType::Pedal             },
    { "lightgun",          ControlType::Lightgun          },
    { "trackball",         ControlType::Trackball         },
    { "doublejoy",         ControlType::DoubleJoy         },
    { "positional",        ControlType::Positional        },
    { "gambling",          ControlType::Gambling          },
    { "hanafuda",          ControlType::Hanafuda          },
    { "doublejoyvertical", ControlType::DoubleJoyVertical },
    { "triplejoy",         ControlType::TripleJoy         },
    { "joystrange",        ControlType::JoyStrange        },
  };
}

ControlType ControlTypeFromString(const String& type)
{
  for (const ControlTypeName& e : sControlTypeNames)
    if (type == e.Name) return e.Type;
  return ControlType::None;
}

const char* ControlTypeToString(ControlType type)
{
  for (const ControlTypeName& e : sControlTypeNames)
    if (e.Type == type) return e.Name;
  return "";
}

// ---- ControllerDatabase ----

ControllerDatabase::ControllerDatabase()
  : mControls(0, sControlGranularity)
  , mPlayers(0, sPlayerGranularity)
{
  LoadDatabase();
}

const GameControlInfo* ControllerDatabase::GetInfo(const FileData& file) const
{
  if (!file.System().Descriptor().IsTrueArcade()) return nullptr;
  auto it = mDatabase.find(file.RomPath().FilenameWithoutExtension().Hash());
  return (it != mDatabase.end()) ? &it->second : nullptr;
}

bool ControllerDatabase::HasControlType(const FileData& file, ControlType type) const
{
  const GameControlInfo* game = GetInfo(file);
  if (game == nullptr) return false;

  for (uint32_t p = 0; p < game->PlayerCount; ++p)
  {
    const PlayerControlInfo& player = mPlayers.ConstRef((int)(game->PlayerIndex + p));
    for (uint32_t c = 0; c < player.ControlCount; ++c)
      if (mControls.ConstRef((int)(player.ControlIndex + c)).Type == type)
        return true;
  }
  return false;
}

ControlType ControllerDatabase::PrimaryControlType(const FileData& file) const
{
  const GameControlInfo* game = GetInfo(file);
  if (game == nullptr || game->PlayerCount == 0) return ControlType::None;

  const PlayerControlInfo& first = mPlayers.ConstRef((int)game->PlayerIndex);
  if (first.ControlCount == 0) return ControlType::None;
  return mControls.ConstRef((int)first.ControlIndex).Type;
}

void ControllerDatabase::LoadDatabase()
{
  String content = Files::LoadFile(Path(sControllerListPath));
  if (content.empty())
  {
    { LOG(LogError) << "[ControllerDB] Could not read " << sControllerListPath; }
    return;
  }

  for (const String& line : content.Split('\n'))
  {
    int colon = line.Find(':');
    if (colon <= 0) continue;
    String rom = line.SubString(0, colon);
    int key = rom.Hash();
    // The generator emits unique ROMs, so a pre-existing key here means a 32-bit
    // hash collision between two different ROMs — surface it rather than silently
    // overwriting (the second ROM keeps the first's controller info).
    if (mDatabase.find(key) != mDatabase.end())
    {
      { LOG(LogWarning) << "[ControllerDB] 32-bit hash collision on '" << rom << "', entry ignored"; }
      continue;
    }

    int playerStart = mPlayers.Count();
    int playerCount = 0;
    int curPlayer   = -1;
    int runStart    = 0; // index in mControls where the current player run began
    int runLength   = 0;

    // Flush the current player run into the shared player pool.
    auto flushPlayer = [&]()
    {
      if (runLength <= 0) return;
      if (runLength > sMaxSliceLength)
      {
        { LOG(LogWarning) << "[ControllerDB] " << rom << ": >255 controls for one player, truncated"; }
        runLength = sMaxSliceLength;
      }
      mPlayers.Add(PlayerControlInfo { (uint32_t)runStart, (uint32_t)runLength });
      ++playerCount;
      runLength = 0;
    };

    bool poolFull = false;
    for (const String& segment : line.SubString(colon + 1).Split(';'))
    {
      if (segment.empty()) continue;
      String::List f = segment.Split('|');
      if ((int)f.size() < 4) continue;

      int player = f[0].AsInt();
      ControlType type = ControlTypeFromString(f[1]);
      if (player <= 0 || type == ControlType::None) continue;

      // Group consecutive same-player segments into one slice (the generator
      // emits a game's controls grouped and sorted by player).
      if (player != curPlayer) { flushPlayer(); curPlayer = player; runStart = mControls.Count(); }

      if (mControls.Count() >= sMaxPoolIndex)
      {
        { LOG(LogError) << "[ControllerDB] control pool exceeded 2^24 entries, ignoring the rest"; }
        poolFull = true;
        break;
      }
      mControls.Add(ControlInfo { type, (uint8_t)f[2].AsInt(), (uint8_t)f[3].AsInt() });
      ++runLength;
    }
    flushPlayer();

    if (playerCount > 0)
    {
      if (playerCount > sMaxSliceLength)
      {
        { LOG(LogWarning) << "[ControllerDB] " << rom << ": >255 players, truncated"; }
        playerCount = sMaxSliceLength;
      }
      mDatabase.insert(key, GameControlInfo { (uint32_t)playerStart, (uint32_t)playerCount });
    }
    if (poolFull) break;
  }

  { LOG(LogDebug) << "[ControllerDB] " << (int)mDatabase.size() << " games, "
                  << mControls.Count() << " controls, " << mPlayers.Count() << " players loaded."; }
}
