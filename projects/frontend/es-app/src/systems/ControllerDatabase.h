//
// Controller database
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <cstdint>
#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>
#include <games/IFilter.h>
#include "SystemData.h"

//! Base control type. 8-bit enum so ControlInfo stays a 3-byte POD. Exhaustive
//! of every type produced by controller.lst. None is the "no data" sentinel.
enum class ControlType : uint8_t
{
  None = 0,          //!< No data / unrecognised
  Joy,               //!< joy (1/2/3/4/5/8/16 ways)
  JoyVertical,       //!< joyvertical
  Mahjong,           //!< mahjong
  Stick,             //!< stick (analog)
  Mouse,             //!< mouse
  Keypad,            //!< keypad
  OnlyButtons,       //!< only_buttons
  Keyboard,          //!< keyboard
  Dial,              //!< dial / spinner
  Paddle,            //!< paddle
  Pedal,             //!< pedal
  Lightgun,          //!< lightgun
  Trackball,         //!< trackball
  DoubleJoy,         //!< doublejoy
  Positional,        //!< positional
  Gambling,          //!< gambling
  Hanafuda,          //!< hanafuda
  DoubleJoyVertical, //!< doublejoyvertical
  TripleJoy,         //!< triplejoy
  JoyStrange,        //!< joystrange
};

//! ControlType <-> string conversions (string form only used at load time)
ControlType ControlTypeFromString(const String& type);
const char* ControlTypeToString(ControlType type);

//! A single control. POD, 3 bytes. Stored contiguously in a single shared pool.
struct ControlInfo
{
  ControlType Type;    //!< Control type
  uint8_t     Ways;    //!< Directional ways (0 if N/A; observed max 16)
  uint8_t     Buttons; //!< Number of buttons (0 if unspecified; observed max 153)
};

//! One player's controls, as a [index, count) slice into the shared control
//! pool. POD, 4 bytes. The player index is positional (1-based slot within its
//! game's player slice) — the actual MAME player number is not stored.
struct PlayerControlInfo
{
  uint32_t ControlIndex : 24; //!< First control in the shared control pool
  uint32_t ControlCount : 8;  //!< Number of consecutive controls (<= 255)
};

//! One arcade game, as a [index, count) slice into the shared player pool.
//! POD, 4 bytes — this is the (tiny) HashMap value.
struct GameControlInfo
{
  uint32_t PlayerIndex : 24; //!< First player in the shared player pool
  uint32_t PlayerCount : 8;  //!< Number of players (<= 255)
};

/*!
 * @brief Reads and deserialises controller.lst, exposing structured
 *        controller information per arcade ROM.
 *
 * Deterministic file format (one line per ROM), one segment per control:
 *   rom:player|type|ways|buttons;player|type|ways|buttons;...
 *
 * Storage is a flattened arena: all ControlInfo of every game live in one shared
 * pool, all PlayerControlInfo in another, and each game is just a 4-byte
 * [index, count) slice. No per-game heap allocation.
 */
class ControllerDatabase : public IFilter
{
  public:
    ControllerDatabase();

    //! True if any control of the game is of the given type
    bool HasControlType(const FileData& file, ControlType type) const;

    //! Primary control type for the game (player 1's first control; None if no data)
    ControlType PrimaryControlType(const FileData& file) const;

    //! Convenience: true if the game uses a dial / spinner
    bool IsDialGame(const FileData& file) const { return HasControlType(file, ControlType::Dial); }

    //! Convenience: true if the game uses a trackball
    bool IsTrackballGame(const FileData& file) const { return HasControlType(file, ControlType::Trackball); }

    //! Set the control type matched by ApplyFilter (used as an IFilter for virtual systems)
    ControllerDatabase& SetFilterType(ControlType type) { mFilterType = type; return *this; }

  private:
    static constexpr const char* sControllerListPath = "/recalbox/system/arcade/flats/controller.lst";

    //! 24-bit index space limit (shared by both pools)
    static constexpr int sMaxPoolIndex = 1 << 24;
    //! 8-bit slice length limit
    static constexpr int sMaxSliceLength = 255;
    //! Pool growth granularity (elements): a handful of reallocs for ~54k/46k entries
    static constexpr int sControlGranularity = 8192;
    static constexpr int sPlayerGranularity  = 8192;

    //! Shared control pool — every game's controls, contiguous
    Array<ControlInfo> mControls;
    //! Shared player pool — every game's players, contiguous
    Array<PlayerControlInfo> mPlayers;
    //! 32-bit rom-name hash -> 4-byte player slice. The rom name itself is not
    //! stored (8-byte pairs). Hash collisions are detected and logged at load.
    HashMap<int, GameControlInfo> mDatabase;

    //! Control type matched by ApplyFilter
    ControlType mFilterType = ControlType::None;

    //! Return the game slice for the given file, nullptr if not found / not arcade
    const GameControlInfo* GetInfo(const FileData& file) const;

    void LoadDatabase();

    /*
     * IFilter implementation
     */

    //! Return true if the given game has a control of the configured filter type
    bool ApplyFilter(const FileData& file) override { return mFilterType != ControlType::None && HasControlType(file, mFilterType); }
};
