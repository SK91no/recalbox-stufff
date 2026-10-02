//
// Created by bkg2k on 30/08/24.
//
#pragma once

#include <guis/menus/base/ItemBase.h>
#include <guis/menus/base/IRatingChanged.h>
#include <guis/menus/base/IMenuInterface.h>
#include "help/Help.h"
#include "utils/locale/LocaleHelper.h"
#include "RatingImage.h"

class ItemRating : public ItemBase
{
  public:
    /*!
     * @brief Constructor
     * @param label Label
     * @param theme Menu theme reference
     * @param interface Callback interface
     * @param identifier Identifier
     * @param icon Icon type
     * @param min Min value
     * @param max Max value
     * @param inc Increment
     * @param value Current value
     * @param defaultValue Default value (for reset)
     * @param suffix Text suffix
     * @param help Optional help string
     * @param unselectable Grayed ?
     */
    ItemRating(Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, IRatingChanged* interface, int identifier, MenuThemeData::MenuIcons::Type icon, float value, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Rating, parent, dataProvider, label, theme, identifier, icon, help, unselectable, helpUnselectable)
      , mRating(dataProvider.IconHeight())
      , mInterface(interface)
      , mInitialValue(value)
      , mValue(value)
      , mSustainedIncrement(0)
      , mSustainedTimeAccumulator(0)
      , mSustainedTimeLimit(0)
    {}

    /*!
     * @brief Constructor
     * @param label Label
     * @param theme Menu theme reference
     * @param interface Callback interface
     * @param identifier Identifier
     * @param min Min value
     * @param max Max value
     * @param inc Increment
     * @param value Current value
     * @param defaultValue Default value (for reset)
     * @param suffix Text suffix
     * @param help Optional help string
     * @param unselectable Grayed ?
     */
    ItemRating(Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, IRatingChanged* interface, int identifier, float value, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Rating, parent, dataProvider, label, theme, identifier, help, unselectable, helpUnselectable)
      , mRating(dataProvider.IconHeight())
      , mInterface(interface)
      , mInitialValue(value)
      , mValue(value)
      , mSustainedIncrement(0)
      , mSustainedTimeAccumulator(0)
      , mSustainedTimeLimit(0)
    {}

    /*
     * Accessors
     */

    //! Get value
    float Value() const { return mValue; }

    /*!
     * @brief Set value
     * @param value new value
     * @param callback True to fire the callback if new value is different from the previous
     */
    void SetValue(float value, bool callback)
    {
      float previous = mValue;
      mValue = Math::clamp(value, sMinimum, sMaximum);
      if (callback && previous != mValue) Fire();
    }

    /*!
     * @brief Check if the value has changed since the initialisation
     * @return True if the value is not the initial value
     */
    bool HasChanged() final { return mValue != mInitialValue; }

  private:
    //! Minimum
    static constexpr float sMinimum = 0.f;
    //! Maximum
    static constexpr float sMaximum = 1.f;
    //! Increment
    static constexpr float sIncrement = .05f;
    //! Time after the first sustained moves
    static constexpr int sSustainedTimeFirst = 200; // 200 ms
    //! Time betwwen 2 sustained moves
    static constexpr int sSustainedTime = 20; // 20 ms

    //! Start line count
    static constexpr int sStarCount = 5;
    //! Rating
    RatingImage mRating;
    //! Interface
    IRatingChanged* mInterface;
    //! Initial Value
    float mInitialValue;
    //! Value
    float mValue;
    //! Percent text size
    int mPercentTextSize;
    //! Sustained incriement for moving knob
    float mSustainedIncrement;
    //! Sustained time accumulator for moving knob
    int mSustainedTimeAccumulator;
    //! Current sustained time limit
    int mSustainedTimeLimit;

    /*
     * Itembase
     */

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) final
    {
      if (event.NorthReleased())
      {
        float previous = mValue;
        mValue = (sMinimum + sMaximum) / 2;
        if (previous != mValue) Fire();
        return true;
      }
      else if (event.L2Released())
      {
        float previous = mValue;
        mValue = sMinimum;
        if (previous != mValue) Fire();
        return true;
      }
      else if (event.R2Released())
      {
        float previous = mValue;
        mValue = sMaximum;
        if (previous != mValue) Fire();
        return true;
      }
      else if (event.AnyLeftPressed())
      {
        mSustainedIncrement = -sIncrement;
        mSustainedTimeLimit = sSustainedTimeFirst;
        return true;
      }
      else if (event.AnyRightPressed())
      {
        mSustainedIncrement = sIncrement;
        mSustainedTimeLimit = sSustainedTimeFirst;
        return true;
      }
      else if (event.AnyLeftReleased() || event.AnyRightReleased())
      {
        mSustainedTimeAccumulator = 0;
        mSustainedIncrement = 0;
        return true;
      }
      return false;
    }

    /*!
     * @brief Called once per frame
     * @param elapsed elaspsed time from last update call
     */
    void Update(int elapsed) final
    {
      if (mSustainedIncrement != 0)
        if (mSustainedTimeAccumulator += elapsed; mSustainedTimeAccumulator > mSustainedTimeLimit)
        {
          mSustainedTimeAccumulator -= mSustainedTimeLimit;
          mSustainedTimeLimit = sSustainedTime;
          float previous = mValue;
          mValue = Math::clamp(mValue + mSustainedIncrement, sMinimum, sMaximum);
          if (previous != mValue) Fire();
        }
    }

    /*!
     * @brief Fill help list
     * @param help Help list to fill
     */
    void CollectHelpItems(Help& help) override
    {
      help.Set(HelpType::LeftRight, _("MOVE"))
          .Set(HelpType::L2R2, _("MIN/MAX"))
          .Set(HelpType::X, _("RESET"));
    }

    /*
     * Overlay
     */

    /*!
     * @brief Apply (draw) an overlay in the given item rectangle and adjust rectangle position/size
     * so that the text won't draw over the overlay if required
     * @param parent Parent menu
     * @param area item rectangle
     * @param text color
     */
    void OverlayDraw(int labelWidth, const Rectangle& area, Colors::ColorRGBA color, bool selected) final
    {
      (void)labelWidth;
      (void)selected;
      /*
      LoadKnob();
      int barStart = (int)(area.Right() - (mDataProvider.Margin() * 2 + mTextSize + mSliderWidth + mKnobWidth / 2));
      int barSize = mSliderWidth - mKnobWidth;
      Renderer::DrawRectangle(barStart, (int)area.CenterY() - 1, barSize, 2, color);
      int offset = (int)((mValue / (mMaximum - mMinimum)) * barSize);
      Renderer::DrawTexture(*mKnob, barStart + offset, area.CenterY() - mKnobHeight / 2, mKnobWidth, mKnobHeight, true, color);
      mDataProvider.MenuTheme().Text().font->RenderDirect(String(mValue, 0).Append(mSuffix), (int)(area.Right() - (mDataProvider.Margin() + mTextSize)), area.Top(), color);
       */
      mRating.Render(area.Right() - mDataProvider.Margin() * 2 - mRating.Width(), (int)area.YCenter() - mRating.Height() / 2, mValue / sMaximum, color);
    }

    /*!
     * @brief Get the left offset (margin to the text) if any
     * @return left offset
     */
    float OverlayLeftOffset(int labelWidth) final { (void)labelWidth; return 0.f; }

    /*!
     * @brief Get the right offset (margin from text to right limit) if any
     * @return right offset
     */
    float OverlayRightOffset(int labelWidth) final
    {
      (void)labelWidth;
      return mRating.Width() + mDataProvider.Margin() * 3;
    }

    //! Fire callback
    void Fire() final { if (mInterface != nullptr) mInterface->MenuRatingMoved(*this, Identifier(), mValue); }

    // Label changed
    void LabelChanged() final { }

    //! Theme has changed - Force this item to recalibrate
    void ThemeHasChanged() final { }
};