//
// Created by bkg2k on 29/08/24.
//
#pragma once

#include <utils/String.h>

// Forward declaration
class ItemEditable;

class IEditableChanged
{
  public:
    //! Default destructor
    virtual ~IEditableChanged() = default;

    /*!
     * @brief Called when an editable text has been updated
     * @param id Component id
     * @param newText changed text
     */
    virtual void MenuEditableChanged(ItemEditable& item, int id, const String& newText) = 0;
};
