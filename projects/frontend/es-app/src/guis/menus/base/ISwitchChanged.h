//
// Created by bkg2k on 22/01/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

// Forward declaration
class ItemSwitch;

class ISwitchChanged
{
  public:
    //! Default destructor
    virtual ~ISwitchChanged() = default;

    /*!
     * @brief Called when a switch component has been updated
     * @param id Component id
     * @param status New status
     */
    virtual void MenuSwitchChanged(const ItemSwitch& item, bool& status, int id) = 0;
};
