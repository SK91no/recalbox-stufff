//
// Created by bkg2k on 24/01/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

class ItemSlider;

class ISliderChanged
{
  public:
    //! Default destructor
    virtual ~ISliderChanged() = default;

    /*!
     * @brief Called when a slider component has been updated
     * @param id Component id
     * @param value New value
     */
    virtual void MenuSliderMoved(ItemSlider& item, int id, float value) = 0;
};
