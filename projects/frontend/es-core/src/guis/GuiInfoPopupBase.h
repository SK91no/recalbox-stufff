//
// Created by xizor on 30/03/18.
//
#pragma once

#include "guis/Gui.h"
#include "components/NinePatchComponent.h"
#include "components/ComponentGrid.h"
#include "WindowManager.h"
#include "utils/math/Vectors.h"
#include "PopupType.h"

class GuiInfoPopupBase : public Gui
{
  public:
    enum class Corner
    {
      TopLeft,
      TopRight,
      BottomRight,
      BottomLeft,
    };

    //! Default destructor
    ~GuiInfoPopupBase() override = default;

    //! Popup must be closed?
    [[nodiscard]] bool TimeOut() const { return mState >= State::Stopped; }

    //! Popup type
    [[nodiscard]] PopupType Type() const { return mType; }

    /*!
     * @brief Set initial target offset, relative to the selected corner
     * @param offset target offset
     */
    void SetOffset(int offset) { mTargetOffset = offset; }

    /*!
     * @brief Construct & initialize components
     */
    void Initialize();

    /*!
     * @brief Check if this instance is self processed and must not be deleted by the window manager
     */
    [[nodiscard]] bool SelfProcessed() const { return mSelfProcessed; }

    /*
     * Component implementation
     */

    void Update(int delta) override;
    void Render(const Transform4x4f& parentTrans) override;

    /*!
     * @brief Stop this popup immediately
     */
    void Stop(int inMs)
    {
      switch(mState)
      {
        case State::FadeIn: mState = State::FadeOut; mFadeTimeAccumulator = sFadeTime - mFadeTimeAccumulator; break;
        case State::Running: mDuration = inMs; break;
        case State::FadeOut:
        case State::Stopped: break;
      }
    }

    /*!
     * @brief Restart this popup immediately
     */
    void Restart()
    {
      switch(mState)
      {
        case State::FadeIn:
        case State::Running: mDuration = -1; break;
        case State::FadeOut: mState = State::FadeIn; mFadeTimeAccumulator = sFadeTime - mFadeTimeAccumulator; mDuration = -1; break;
        case State::Stopped: mState = State::FadeIn; mFadeTimeAccumulator = 0; mDuration = -1; break;
      }
    }

  protected:
    /*!
     * @brief Protected constructor
     * @param window Window manager
     * @param duration Popup duration
     * @param icon Icon type
     * @param gridWidth inner Grid width
     * @param gridHeight inner Grid height
     * @param widthRatio Ratio applied to the popup width
     */
    GuiInfoPopupBase(WindowManager& window, bool selfProcessed, int duration, PopupType icon, int gridWidth, int gridHeight, float widthRatio);

    /*!
     * @brief Allow inherited classes to acces Grid
     * @return Grid component
     */
    ComponentGrid& Grid() { return mGrid; }

    //! Check if the class is initialized
    [[nodiscard]] bool Initialized() const { return mInitialized; }

    /*!
     * @brief Inherited class must use this method to add their components into the grid
     * @param window Window manager
     * @param grid  Component grid
     * @param maxWidth max width
     * @param maxHeight max height
     * @return Real height
     */
    virtual float AddComponents(WindowManager& window, ComponentGrid& grid, float maxWidth, float maxHeight, int paddingX, int paddingY) = 0;

    /*!
     * @brief Slide target offset from the given amount of pixels
     * @param offset
     */
    void SlideOffset(int size);

  private:
    //! Fade time in ms
    static constexpr int sFadeTime = 500;

    //! Popup status
    enum class State
    {
      FadeIn,  //!< The popup is apearing and fading in
      Running, //!< Normal running state
      FadeOut, //!< The popup is fading out and disappear
      Stopped, //!< Popup is topped waiting for display list removal & destruction
    };

    ComponentGrid mGrid;
    NinePatchComponent mFrame;
    unsigned int mFrameColor;
    PopupType mType;
    Corner mCorner;
    int mTargetOffset;
    int mDuration;
    int mFadeTimeAccumulator;
    int mMaxAlpha;
    float mWidthRatio;
    State mState;
    bool mInitialized;
    bool mSelfProcessed;

    void SetState(State state)
    {
      mState = state;
      mFadeTimeAccumulator = 0;
    }
};