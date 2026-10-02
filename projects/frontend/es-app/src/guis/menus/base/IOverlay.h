//
// Created by bkg2k on 22/08/24.
//
#pragma once

#include <rendering/opengl/Rectangle.h>
#include <rendering/opengl/Colors.h>

// Forward declarations
class Menu;
class ItemBase;
class IMenuInterface;

class IOverlay
{
  public:
    //! default destructor
    virtual ~IOverlay() = default;

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param labelWidth Menu label width in pixel
     * @param area item rectangle
     * @param color text color
     * @param selected True if the item is selected, false otherwise
     */
    virtual void OverlayDraw(int labelWidth, const Rectangle& area, Colors::ColorRGBA color, bool selected) = 0;

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param labelWidth Menu label width in pixel
     * @return left offset
     */
    virtual float OverlayLeftOffset(int labelWidth) = 0;

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param labelWidth Menu label width in pixel
     * @return right offset
     */
    virtual float OverlayRightOffset(int labelWidth) = 0;
};