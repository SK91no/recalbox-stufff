//
// Created by bkg2k on 02/01/2020.
//
#pragma once

#include <guis/Gui.h>
#include <components/TextListComponent.h>
#include <components/BusyComponent.h>
#include <bios/Bios.h>
#include <bios/BiosList.h>
#include <components/ButtonComponent.h>

class GuiBiosMd5 : public Gui
{
  private:
    //! Text color index
    static constexpr signed char sColorIndexNormal = 0;
    //! Half Text color index
    static constexpr signed char sColorIndexHalf = 1;
    //! Transparent Red
    static constexpr signed char sColorIndexRed = 2;
    //! Transparent Yellow
    static constexpr signed char sColorIndexGray = 3;
    //! Transparent Green
    static constexpr signed char sColorIndexGreen = 4;


    //! Global component grid for easy positioning
    ComponentGrid mGrid;

    //! Text title
    std::shared_ptr<TextComponent> mTitle;
    //! Header
    std::shared_ptr<TextComponent> mHeader;

    //! MD5 list
    std::shared_ptr<TextListComponent<bool>> mList;

    /*!
     * @brief Called when sizing window
     */
    void onSizeChanged() override;

    /*!
     * @brief Process input events
     * @param event Input event
     * @return True if the method consumed the event
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Set help prompts
     * @param help Help structure to fill in
     * @return True
     */
    bool CollectHelpItems(Help& help) override;

    /*!
     * @brief Build common UI
     */
    void BuildUI();

  public:
    /*!
     * @brief Single bios constructor
     * @param window Main window
     * @param bios Single bios object
     */
    GuiBiosMd5(WindowManager& window, const Bios& bios);

    /*!
     * @brief Multi-bios constructor
     * @param window Main window
     * @param biosList Bios list object
     */
    GuiBiosMd5(WindowManager& window, const BiosList& biosList);
};
