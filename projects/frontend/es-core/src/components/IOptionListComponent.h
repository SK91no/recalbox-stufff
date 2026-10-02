//
// Created by bkg2k on 23/01/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

template<typename T> class IOptionListComponent
{
  public:
    /*!
     * @brief Called when an editable component has been updated
     * @param id Component id
     * @param index selected index
     * @param value New value
     * @param quickChange True if the user made a quick change using lest/right direction
     */
    virtual void OptionListComponentChanged(int id, int index, const T& value, bool quickChange) = 0;
};