//
// Created by bkg2k on 09/09/25.
//
#pragma once

class String;

class PadTypes
{
  public:
    //! Pad-style identifier. Drives both the on-screen pad image and the
    //! "gamepadtype" string persisted alongside each device's binding.
    enum Type
    {
      HeightBitdo, //!< 8bitdo
      XBox,        //!< XBox pad type
      Playstation, //!< PS Style pads
      Arcade8,     //!< Arcade panel 8 buttons
      Arcade6,     //!< Arcade panel 6 buttons
      Arcade4,     //!< Arcade panel 4 buttons
      Gamecube,    //!< Gamecube pads
      Megadrive6,  //!< Megadrive 6 buttons
      Megadrive3,  //!< Megadrive 3 buttons
      n64,         //!< n64 pads
      __Count,
    };

    //! Stringized form persisted in the input XML / recalbox.conf.
    static const String& ToString(Type type);

    //! Inverse of ToString. Defaults to HeightBitdo when nothing matches.
    static Type FromString(const String& type);

    //! Guess from device name (case-insensitive substring match).
    //! Returns Type::__Count when nothing matches.
    static Type FromControllerName(const String& deviceName);

  private:
    PadTypes() = delete;
};
