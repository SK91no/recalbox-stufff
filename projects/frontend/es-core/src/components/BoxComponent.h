//
// Created by bkg2k on 11/12/23.
//
#pragma once

#include "utils/math/Transform4x4f.h"
#include "WindowManager.h"
#include <components/base/ThemableComponent.h>

class BoxComponent : public ThemableComponent
{
  public:
    explicit BoxComponent(WindowManager& window)
      : ThemableComponent(window)
      , mTopLeftColor(0)
      , mTopRightColor(0)
      , mBottomLeftColor(0)
      , mBottomRightColor(0)
    {
    }

    /*
     * Component override
     */

    /*!
     * @brief Render box on screen
     * @param trans Parent transformation
     */
    void Render(const Transform4x4f& trans) override;

    /*!
     * @brief Set box 4 corners color
     * @param color RGBA color
     */
    void SetColor(Colors::ColorRGBA color)
    {
      mTopLeftColor = color;
      mTopRightColor = color;
      mBottomLeftColor = color;
      mBottomRightColor = color;
    }

    /*!
     * @brief Set box top border color
     * @param color RGBA color
     */
    void SetTopColor(Colors::ColorRGBA color)
    {
      mTopLeftColor = color;
      mTopRightColor = color;
    }

    /*!
     * @brief Set box bottom border color
     * @param color RGBA color
     */
    void SetBottomColor(Colors::ColorRGBA color)
    {
      mBottomLeftColor = color;
      mBottomRightColor = color;
    }

    /*!
     * @brief Set box left border color
     * @param color RGBA color
     */
    void SetLeftColor(Colors::ColorRGBA color)
    {
      mTopLeftColor = color;
      mBottomLeftColor = color;
    }

    /*!
     * @brief Set box right border color
     * @param color RGBA color
     */
    void SetRightColor(Colors::ColorRGBA color)
    {
      mTopRightColor = color;
      mBottomRightColor = color;
    }

    /*!
     * @brief Set box top/left corner color
     * @param color RGBA color
     */
    void SetTopLeftColor(Colors::ColorRGBA color) { mTopLeftColor = color; }

    /*!
     * @brief Set box top/right corner color
     * @param color RGBA color
     */
    void SetTopRightColor(Colors::ColorRGBA color) { mTopRightColor = color; }

    /*!
     * @brief Set box bottom/left corner color
     * @param color RGBA color
     */
    void SetBottomLeftColor(Colors::ColorRGBA color) { mBottomLeftColor = color; }

    /*!
     * @brief Set box bottom/right corner color
     * @param color RGBA color
     */
    void SetBottomRightColor(Colors::ColorRGBA color) { mBottomRightColor = color; }

    /*!
     * @brief Get box top/left color
     * @return Box top/left color
     */
    [[nodiscard]] Colors::ColorRGBA TopLeftColor() const { return mTopLeftColor; }

    /*!
     * @brief Get box top/right color
     * @return Box top/right color
     */
    [[nodiscard]] Colors::ColorRGBA TopRightColor() const { return mTopRightColor; }

    /*!
     * @brief Get box bottom/left color
     * @return Box bottom/left color
     */
    [[nodiscard]] Colors::ColorRGBA BottomLeftColor() const { return mBottomLeftColor; }

    /*!
     * @brief Get box bottom/right color
     * @return Box bottom/right color
     */
    [[nodiscard]] Colors::ColorRGBA BottomRightColor() const { return mBottomRightColor; }

    /*
     * Themable implementation
     */

    /*!
     * @brief Called when a theme element is applyed on the current component.
     * @param element Element instance
     * @param properties Properties to update
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Box; }

  private:
    //! Color
    Colors::ColorRGBA mTopLeftColor;
    Colors::ColorRGBA mTopRightColor;
    Colors::ColorRGBA mBottomLeftColor;
    Colors::ColorRGBA mBottomRightColor;
};
