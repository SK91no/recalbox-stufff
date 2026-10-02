//
// Created by bkg2k on 01/06/25.
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

/*!
 * @brief Store the context sensitivity after analysis.
 * The component may react to global context or system/game contexts
 * Also displayed system may be different from real system
 */
enum class ThemeContexts : unsigned char
{
  None                  = 0, //!< Not reactive to anything: does not contains any variable
  UserVariable          = 1, //!< For variable processing only, no refresh required
  Global                = 2, //!< Global context, must refresh everytime
  DisplayedSystem       = 4, //!< Displayed system (real + virtual)
  DisplayedSystemOffset = 8, //!< Displayed system + offset (real + virtual)
  GameAndSystem         = 16, //!< Game context and its real system
  All                   = 0xFF
};

DEFINE_BITFLAG_ENUM(ThemeContexts, unsigned char);