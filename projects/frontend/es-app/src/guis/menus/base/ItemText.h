//
// Created by bkg2k on 22/08/24.
//
#pragma once

#include <guis/menus/base/ItemBase.h>
#include "IMenuInterface.h"

//! Submenu item
class ItemText : public ItemBase
{
  public:
    ItemText(Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, int identifier, const String& text, Colors::ColorRGBA forcedColor, const String& help)
      : ItemBase(Type::Text, parent, dataProvider, label, theme, identifier, help, false, String::Empty)
      , mOriginalText(text)
      , mTextWidth(0)
      , mForcedColor(forcedColor)
    {}

    //! Get text
    const String& Text() const { return mOriginalText; }

    /*!
     * @brief Set new text
     * @param newText new string
     */
    void SetText(const String& newText)
    {
      mOriginalText = newText;
      mTextWidth = 0; // Allow right part to self calibrate
    }

    /*!
     * @brief Text item has no value. Always return false
     * @return false
     */
    bool HasChanged() final { return false; }

  private:
    //! Original Text
    String mOriginalText;
    //! Cut Text
    String mShortenedText;
    //! Text width
    int mTextWidth;
    //! Forced color
    Colors::ColorRGBA mForcedColor;

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) override { (void)event; return false; }

    /*!
     * @brief Called once per frame
     * @param elapsed elaspsed time from last update call
     */
    void Update(int elapsed) final { (void)elapsed; }

    /*!
     * @brief Fill help list
     * @param help Help list to fill
     */
    void CollectHelpItems(Help& help) override { (void)help; }

    /*
     * IOverlay implementation
     */

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param menudata Parent menu
     * @param area item rectangle
     * @param text color
     */
    void OverlayDraw(int labelWidth, const Rectangle& area, Colors::ColorRGBA color, bool selected) final
    {
      (void)labelWidth;
      (void)selected;
      int x = (int)(area.Right() - mDataProvider.Margin() - mTextWidth);
      mTheme.Text().font->DrawText(mShortenedText, Rectangle((float)x, area.Top(), (float)mTextWidth, area.Height()), mForcedColor != 0 ? mForcedColor : color, Alignment::CenterLeft);
    }

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @param menudata Parent menu
     * @return left offset
     */
    float OverlayLeftOffset(int labelWidth) final { (void)labelWidth; return 0.f; }

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @param menudata Parent menu
     * @return right offset
     */
    float OverlayRightOffset(int labelWidth) final
    {
      if (mTextWidth == 0)
      {
        mTextWidth = mTheme.Text().font->TextWidth(mOriginalText);
        if (CalibrateLeftAndRightParts(labelWidth, mTextWidth, mDataProvider.Margin() * 2))
          mShortenedText = mTheme.Text().font->TextEllipsis(mOriginalText, mTextWidth);
        else
          mShortenedText = mOriginalText;
      }
      return mTextWidth + mDataProvider.Margin() * 2;
    }

    //! Fire callback
    void Fire() final {}

    //! Label changed - Force this item to recalibrate
    void LabelChanged() final { mTextWidth = 0; }

    //! Theme has changed - Force this item to recalibrate
    void ThemeHasChanged() final { mTextWidth = 0; }
};

