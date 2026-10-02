//
// Created by bkg2k on 08/04/2026.
//
#pragma once

#include <guis/Gui.h>
#include <components/NinePatchComponent.h>
#include <components/ComponentGrid.h>
#include <components/TextComponent.h>
#include <components/MarkdownText.h>

class GuiInformation : public Gui
{
  public:
    /*!
     * @brief Constructor
     * @param window Main window
     */
    explicit GuiInformation(WindowManager& window);

  private:
    //! Output file path
    static constexpr const char* sOutputFile = "/recalbox/share/system/SystemInformation.md";
    //! Network Output file path
    static constexpr const char* sNetworkFile = "\\\\RECALBOX\\system\\SystemInformation.md";

    //! Window background
    NinePatchComponent mBackground;
    //! Main layout grid (3 cols × 3 rows)
    ComponentGrid mGrid;

    //! Title label
    std::shared_ptr<TextComponent> mTitle;
    //! Body markdown content
    std::shared_ptr<MarkdownText> mText;

    /*!
     * @brief Collect and format all system information as a Markdown string
     * @return Markdown-formatted system information
     */
    static String BuildSystemInfo();

    /*!
     * @brief Recompute column/row percentages when the window is resized
     */
    void onSizeChanged() override;

    /*!
     * @brief Process input events
     * @param event Input event
     * @return True if the event was consumed
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Populate the help bar
     * @param help Help structure to fill
     * @return True
     */
    bool CollectHelpItems(Help& help) override;
};
