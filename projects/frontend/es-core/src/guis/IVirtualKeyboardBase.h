//
// Created by bkg2k on 27/02/25.
//
#pragma once

//! Character set filter restricting which characters the VK accepts
enum class VKCharFilter
{
  All,          //!< Any character allowed
  Alphanumeric, //!< Letters (a-z, A-Z) + digits (0-9) only
  Numeric,      //!< Digits (0-9) only
  FullNumeric,  //!< Digits (0-9), minus sign and decimal point
};

class IVirtualKeyboardBase
{
  public:
    ~IVirtualKeyboardBase() = default;
};
