//
// Created by bkg2k on 19/11/2019.
//
#pragma once

#include <components/base/Component.h>
#include "guis/menus/base/NinePatchImage.h"

class MenuThemeData;
class NinePatchImage;

class Gui : public Component
{
  private:
    //! Menu data
    const MenuThemeData& mTheme;
    //! Frame imag epath
    Path mFramePath;
    //! Common background
    NinePatchImage mFrame;

    //! When set, let the main processing delete safely this GUI
    bool mGuiClosePending;

  protected:
    //! Renderer access
    Renderer& mRenderer;

  public:
    explicit Gui(WindowManager& window);

    /*!
     * @brief Center the window
     */
    void CenterGui();

    /*!
     * @brief Close the GUI
     */
    void Close() { mGuiClosePending = true; }

    /*!
     * @brief Get deletion status
     * @return True if the current GUI is pending for deletion
     */
    [[nodiscard]] bool IsPendingForDeletion() const { return mGuiClosePending; }

    /*!
     * @brief Check if the current UI is on top of the screen
     * @return True if the current UI is the first visible
     */
    [[nodiscard]] bool AmIOnTopOfScreen() const;

    /*!
     * @brief Override and return true if the GUI is an overlay and the underlying GUI must me displayed under.
     * @return True if the GUI is an overlay
     */
    [[nodiscard]] virtual bool IsOverlay() const { return false; }

    /*!
     * @brief If true, the help system is rendered under this gui
     * @return True if the GUI must be rendered over the Help system
     */
    [[nodiscard]] virtual bool MustRenderOverHelpSystem() const { return false; }

    /*!
     * @brief Allow a gui to tell the system not to interrupt by popup windows
     * @return True of the windows does not want to be interrupted, false otherwise
     */
    [[nodiscard]] virtual bool DoNotDisturb() const { return false; }

    /*!
     * @brief Override rendering to draw custom background
     * @param parentTrans parent transformation
     */
    void Render(const Transform4x4f& parentTrans) override;
};
