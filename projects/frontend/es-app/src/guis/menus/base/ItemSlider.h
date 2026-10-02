//
// Created by bkg2k on 20/08/24.
//
#pragma once

#include <guis/menus/base/ItemBase.h>
#include <guis/menus/base/ISliderChanged.h>
#include "help/Help.h"
#include "utils/locale/LocaleHelper.h"
#include <themes/MenuThemeDataCache.h>
#include <input/InputCompactEvent.h>

//! Submenu item
class ItemSlider : public ItemBase
{
  public:
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
    ItemSlider(Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, ISliderChanged* interface, int identifier, float min, float max, float inc, float value, float defaultValue, const String& suffix, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Slider, parent, dataProvider, label, theme, identifier, help, unselectable, helpUnselectable)
      , mSuffix(suffix)
      , mInterface(interface)
      , mMinimum(min)
      , mMaximum(max)
      , mIncrement(inc)
      , mInitialValue(value)
      , mValue(value)
      , mDefaultValue(defaultValue)
      , mKnobWidth(0)
      , mKnobHeight(0)
      , mSliderWidth(0)
      , mTextSize(0)
      , mSustainedIncrement(0)
      , mSustainedTimeAccumulator(0)
      , mSustainedTimeLimit(0)
      , mSustainedFired(false)
    {}

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
    ItemSlider(Menu& parent, IMenuInterface& dataProvider, const String& label, const MenuThemeData& theme, ISliderChanged* interface, int identifier, MenuThemeData::MenuIcons::Type icon, float min, float max, float inc, float value, float defaultValue, const String& suffix, const String& help, bool unselectable, const String& helpUnselectable)
      : ItemBase(Type::Slider, parent, dataProvider, label, theme, identifier, icon, help, unselectable, helpUnselectable)
      , mSuffix(suffix)
      , mInterface(interface)
      , mMinimum(min)
      , mMaximum(max)
      , mIncrement(inc)
      , mInitialValue(value)
      , mValue(value)
      , mDefaultValue(defaultValue)
      , mKnobWidth(0)
      , mKnobHeight(0)
      , mSliderWidth(0)
      , mTextSize(0)
      , mSustainedIncrement(0)
      , mSustainedTimeAccumulator(0)
      , mSustainedTimeLimit(0)
      , mSustainedFired(false)
    {}

    /*
     * Accessors
     */

    //! Get maximum value
    float Maximum() const { return mMaximum; }
    //! Get minimum value
    float Minimum() const { return mMinimum; }
    //! Get increment
    float Increment() const { return mIncrement; }
    //! Get value
    float Value() const { return mValue; }
    //! Get default value
    float DefaultValue() const { return mDefaultValue; }

    /*!
     * @brief Set value
     * @param value new value
     * @param callback True to fire the callback if new value is different from the previous
     */
    void SetValue(float value, bool callback)
    {
      float previous = mValue;
      mValue = Math::clamp(value, mMinimum, mMaximum);
      if (callback && previous != mValue) Fire();
    }

    /*!
     * @brief Check if the value has changed since the initialisation
     * @return True if the value is not the initial value
     */
    bool HasChanged() final { return mValue != mInitialValue; }

  private:
    //! Time after the first sustained moves
    static constexpr int sSustainedTimeFirst = 200; // 200 ms
    //! Time betwwen 2 sustained moves
    static constexpr int sSustainedTime = 20; // 20 ms

    //! On resource
    Texture mKnob;
    //! Suffix
    String mSuffix;
    //! Interface
    ISliderChanged* mInterface;
    //! Value min
    float mMinimum;
    //! Value max
    float mMaximum;
    //! Increment
    float mIncrement;
    //! Initial value
    float mInitialValue;
    //! Value
    float mValue;
    //! Default value
    float mDefaultValue;
    //! Knob width
    int mKnobWidth;
    //! Knob height
    int mKnobHeight;
    //! Slider width
    int mSliderWidth;
    //! Text size
    int mTextSize;
    //! Sustained incriement for moving knob
    float mSustainedIncrement;
    //! Sustained time accumulator for moving knob
    int mSustainedTimeAccumulator;
    //! Current sustained time limit
    int mSustainedTimeLimit;
    //! Sustained fires (at least one time)
    bool mSustainedFired;

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
        mValue = mDefaultValue;
        if (previous != mValue) Fire();
        mSustainedFired = false;
        return true;
      }
      if (event.L2Released())
      {
        float previous = mValue;
        mValue = mMinimum;
        if (previous != mValue) Fire();
        mSustainedFired = false;
        return true;
      }
      if (event.R2Released())
      {
        float previous = mValue;
        mValue = mMaximum;
        if (previous != mValue) Fire();
        mSustainedFired = false;
        return true;
      }
      if (event.AnyLeftPressed())
      {
        mSustainedIncrement = -mIncrement;
        mSustainedTimeLimit = sSustainedTimeFirst;
        mSustainedFired = false;
        return true;
      }
      if (event.AnyRightPressed())
      {
        mSustainedIncrement = mIncrement;
        mSustainedTimeLimit = sSustainedTimeFirst;
        mSustainedFired = false;
        return true;
      }
      if (event.AnyLeftReleased() || event.AnyRightReleased())
      {
        if (!mSustainedFired)
        {
          float previous = mValue;
          mValue = Math::clamp(mValue + mSustainedIncrement, mMinimum, mMaximum);
          if (previous != mValue) Fire();
        }
        mSustainedTimeAccumulator = 0;
        mSustainedIncrement = 0;
        mSustainedFired = false;
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
          mValue = Math::clamp(mValue + mSustainedIncrement, mMinimum, mMaximum);
          if (previous != mValue) { Fire(); mSustainedFired = true; }
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
      LoadKnob();
      int barStart = (int)area.Right() - ((int)mDataProvider.Margin() * 2 + mTextSize + mSliderWidth + (mKnobWidth / 2));
      int barSize = mSliderWidth;
      Renderer::DrawRectangle(barStart, (int)area.YCenter() - 1, barSize, 2, color);
      int offset = (int)(((mValue - mMinimum) / (mMaximum - mMinimum)) * (float)barSize);
      mKnob.Render(barStart + offset - (mKnobWidth / 2), (int)area.YCenter() - mKnobHeight / 2, mKnobWidth, mKnobHeight, true, false, color);
      mTheme.Text().font->DrawText(String(mValue, 0).Append(mSuffix), Rectangle(Math::round(area.Right() - (mDataProvider.Margin() + mTextSize)), area.Top(), mDataProvider.Margin() + mTextSize, area.Height()), color, Alignment::Center);
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
      LoadKnob();
      return (float)mSliderWidth;
    }

    //! Fire callback
    void Fire() final { if (mInterface != nullptr) mInterface->MenuSliderMoved(*this, Identifier(), mValue); }

    //! Label changed
    void LabelChanged() final { }

    //! Theme has changed - reload
    void ThemeHasChanged() final { mKnobHeight = 0; }

    //! Load knob icon
    void LoadKnob()
    {
      if (mKnobHeight != 0) return;

      // Text
      mTextSize = (int)mTheme.Text().font->TextWidth(String((int)mMaximum).Append(mSuffix));

      // Lnob
      mKnobHeight = (int)(mDataProvider.FontHeight() * 0.7f);
      mKnob = mDataProvider.Cache().GetElement(MenuThemeData::IconElement::Type::Knob, 0, mKnobHeight);
      mKnobWidth = (int)((mKnob.Width() * (float)mKnobHeight) / mKnob.Height());
      mSliderWidth = (int)Math::max(mDataProvider.ItemWidth() / 5, mDataProvider.Margin() * 2 + (float)mKnobWidth * 2) + (int)mDataProvider.Margin() + mTextSize;
    }
};
