//
// Created by bkg2k on 26/08/24.
//
#pragma once

class MenuColors
{
  public:
    //! List's Color index: Selectable text
    static constexpr int sSelectableColor = 0;
    //! List's Color index: Selected text
    static constexpr int sSelectableSelectedColor = 1;
    //! List's Color index: Unselectable text
    static constexpr int sUnselectableColor = 2;
    //! List's Color index: Unselectable selected text
    static constexpr int sUnselectableSelectedColor = 3;
    //! List's Color index: Header Background
    static constexpr int sHeaderColor = 4;
    //! List's Color index: Header Background
    static constexpr int sHeaderSelectedColor = 5;

    /*
     * Color Helper
     */

    //! Change the alpha component of the given color to 25% of its original value
    static Colors::ColorRGBA Alpha25Percent(Colors::ColorRGBA color) { return (color & 0xFFFFFF00) | ((color & 0xFF) >> 2); }

    //! Change the alpha component of the given color to 50% of its original value
    static Colors::ColorRGBA Alpha50Percent(Colors::ColorRGBA color) { return (color & 0xFFFFFF00) | ((color & 0xFF) >> 1); }

    /*!
     * @brief Blend color a and color b, 50% each
     */
    static Colors::ColorRGBA MergeColor(Colors::ColorRGBA a, Colors::ColorRGBA b)
    {
      int ra = a >> 24;
      int ga = (a >> 16) & 0xFF;
      int ba = (a >> 8) & 0xFF;
      int aa = a & 0xFF;
      int rb = b >> 24;
      int gb = (b >> 16) & 0xFF;
      int bb = (b >> 8) & 0xFF;
      int ab = b & 0xFF;
      return (((ra + rb) >> 1) << 24) | (((ga + gb) >> 1) << 16) | (((ba + bb) >> 1) << 8) | ((aa + ab) >> 1);
    }
};