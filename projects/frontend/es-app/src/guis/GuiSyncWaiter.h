//
// Created by bkg2k on 26/10/2020.
//
#pragma once

#include <guis/Gui.h>
#include <components/BusyComponent.h>
#include <WindowManager.h>
#include <Renderer.h>

/*!
 * @brief Syncnronized operations waiter
 */
class GuiSyncWaiter : public Gui
{
  public:
    explicit GuiSyncWaiter(WindowManager& window, const String& message)
      : Gui(window)
      , mBusyAnim(window)
      , mTimeReference(0)
      , mShown(false)
    {
      // Configure animation
      mBusyAnim.setSize(Renderer::Instance().DisplayWidthAsFloat(), Renderer::Instance().DisplayHeightAsFloat());
      mBusyAnim.SetText(message);
    }

    void Show()
    {
      if (!mShown) mWindow.pushGui(this);
      mWindow.RenderAll(false);
      mShown = true;
      mTimeReference = (int)SDL_GetTicks();
      SDL_GL_SetSwapInterval(0);
    }

    void Refresh()
    {
      if (mShown)
        mWindow.RenderAll(false);
    }

    void Hide()
    {
      if (mShown)
        if (mWindow.RemoveGui(this))
          mWindow.RenderAll(false);
      mShown = false;
      SDL_GL_SetSwapInterval(1);
    }

  protected:
    /*!
     * @brief Update components
     * @param deltaTime delta ms from the previous frame
     */
    void Update(int deltaTime) override
    {
      (void)deltaTime;
      // This waiter is not intended to be executed in the main loop.
      Hide();
    }

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
                         Renderer::Instance().DisplayWidthAsFloat(), mBusyAnim.RealHeight() * 1.6f, 0x00000080);

      mBusyAnim.Render(trans);
    }

    /*!
     * @brief This window is an overlay and does not hide underlying windows
     * @return Always true
     */
    [[nodiscard]] bool IsOverlay() const override { return true; }

  private:
    //! Busy animation
    BusyComponent mBusyAnim;
    //! Time reference
    int mTimeReference;
    //! Show status
    bool mShown;
};

