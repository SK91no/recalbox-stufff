//
// Created by bkg2k on 16/01/24.
//
#pragma once

#include <utils/String.h>

class IGlobalVariableResolver
{
  public:
    //! Default destructor
    virtual ~IGlobalVariableResolver() = default;

    /*!
     * @brief Resolve variables in the given string
     * @param string String in which to resolve variables
     */
    virtual void ResolveVariableIn(String& string) = 0;

    //! Has a CRT adapter connected?
    [[nodiscard]] virtual bool HasCrt() const = 0;

    //! Has a CRT adapter active?
    [[nodiscard]] virtual bool HasCrtAttached() const = 0;

    //! Has any CRT hat installed (regardless of resolution / screen state)?
    [[nodiscard]] virtual bool HasCrtHatInstalled() const = 0;

    //! Has a CRT interlaced support
    [[nodiscard]] virtual bool HasCrtInterlaced() const = 0;

    //! Has an RGB Dual adapter active?
    [[nodiscard]] virtual bool HasRRGBD() const = 0;

    //! Has an RGB Dual 2 adapter active?
    [[nodiscard]] virtual bool HasRRGBD2() const = 0;

    //! Has a JVS adapter active?
    [[nodiscard]] virtual bool HasJVS() const = 0;

    //! Has an RGB Jamma adapter active?
    [[nodiscard]] virtual bool HasJamma() const = 0;

    //! Is it a PC ?
    [[nodiscard]] virtual bool IsPC() const = 0;
    //! Is it a Pi ?
    [[nodiscard]] virtual bool IsPi() const = 0;
    //! Is it an odroid board ?
    [[nodiscard]] virtual bool IsOdroid() const = 0;
    //! Is it an anbernic board
    [[nodiscard]] virtual bool IsAnbernic() const = 0;

    //! Is in TATE mode?
    [[nodiscard]] virtual bool IsTate() const = 0;
    //! Is in right TATE mode?
    [[nodiscard]] virtual bool IsTateRight() const = 0;
    //! Is in left TATE mode?
    [[nodiscard]] virtual bool IsTateLeft() const = 0;

    //! Is resolution QVGA or less?
    [[nodiscard]] virtual bool IsQVGA() const = 0;

    //! Is Resolution between QVGA (excluded) & VGA (included)?
    [[nodiscard]] virtual bool IsVGA() const = 0;

    //! Is Resolution between VGA (excluded) & HD (included)?
    [[nodiscard]] virtual bool IsHD() const = 0;

    //! Is Resolution higher than HD?
    [[nodiscard]] virtual bool IsFHD() const = 0;

    //! Is Resolution higher than HD?
    [[nodiscard]] virtual bool IsBartopModeOrHigher() const = 0;

    //! Is Resolution higher than HD?
    [[nodiscard]] virtual bool IsNoMenuMode() const = 0;

    //! Is HDMI?
    [[nodiscard]] virtual bool HasHDMI() const = 0;

    //! Is CardReader?
    [[nodiscard]] virtual bool HasCardReaderAvailable() const = 0;

    //! Card Plugged?
    [[nodiscard]] virtual bool HasCardReaderPlugged() const = 0;
};
