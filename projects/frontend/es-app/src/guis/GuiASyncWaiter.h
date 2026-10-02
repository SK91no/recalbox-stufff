//
// Created by bkg2k on 26/10/2020.
//
#pragma once

#include <guis/Gui.h>
#include <components/BusyComponent.h>
#include <components/ProgressBarComponent.h>
#include <WindowManager.h>

/*!
 * @brief Syncnronized operations waiter
 */
class GuiASyncWaiter : public Gui
{
  public:
    explicit GuiASyncWaiter(WindowManager& window, const String& message)
      : Gui(window)
      , mBusyAnim(window)
      , mProgress(window, 100)
      , mTimeReference(0)
      , mShown(false)
    {
      // Configure animation
      mBusyAnim.setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
      mBusyAnim.SetText(message);
      mProgress.setSize(Renderer::Instance().DisplayWidthAsFloat() / 2, Renderer::Instance().DisplayHeightAsFloat() / 40);
      mProgress.setPosition(Renderer::Instance().DisplayWidthAsFloat() / 4, (Renderer::Instance().DisplayHeightAsFloat() + mBusyAnim.RealHeight()) / 2 + 4);
      mProgress.setCurrentValue(0);
    }

    void SetProgressText(const String& text) { mBusyAnim.SetText(text); }

    void SetSubText(const String& text) { mBusyAnim.SetSecondaryText(text); }

    void SetProgress(int progress) { mProgress.setCurrentValue(progress); mShown = true; }

  protected:
    /*!
     * @brief Draw
     * @param parentTrans Parent transformation
     */
    void Render(const Transform4x4f& parentTrans) override
    {
      int newTimeReference = (int)SDL_GetTicks();
      mBusyAnim.Update(newTimeReference - mTimeReference);
      mTimeReference = newTimeReference;
      Transform4x4f trans = parentTrans * getTransform();
      Renderer::SetMatrix(trans);

      Renderer::DrawRectangle(0.f, (Renderer::Instance().DisplayHeightAsFloat() - mBusyAnim.RealHeight() * 1.6f) / 2.0f,
                              Renderer::Instance().DisplayWidthAsFloat(), mBusyAnim.RealHeight() * 1.6f + (Renderer::Instance().DisplayHeightAsFloat() / 40), 0x00000080);

      mBusyAnim.Render(trans);
      if (mShown) mProgress.Render(trans);
    }

    /*!
     * @brief This window is an overlay and does not hide underlying windows
     * @return Always true
     */
    [[nodiscard]] bool IsOverlay() const override { return true; }

  private:
    //! Busy animation
    BusyComponent mBusyAnim;
    //! Progress bar
    ProgressBarComponent mProgress;
    //! Time reference
    int mTimeReference;
    //! Show status
    bool mShown;
};

