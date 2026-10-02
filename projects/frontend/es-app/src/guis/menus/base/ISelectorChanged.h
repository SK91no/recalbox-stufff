//
// Created by bkg2k on 24/08/24.
//
#pragma once

#include <vector>

template<typename T> class ItemSelectorBase;

template<typename T> class ISingleSelectorChanged
{
  public:
    //! Default destructor
    virtual ~ISingleSelectorChanged() = default;

    /*!
     * @brief Called when a single selector changed
     * @param id Component id
     * @param value new value selected
     */
    virtual void MenuSingleChanged(ItemSelectorBase<T>& item, int id, int index, const T& value) = 0;
};

template<typename T> class IMultiSelectorChanged
{
  public:
    //! Default destructor
    virtual ~IMultiSelectorChanged() = default;

    /*!
     * @brief Called when a single selector changed
     * @param id Component id
     * @param value array of selected value
     */
    virtual void MenuMultiChanged(const ItemSelectorBase<T>& item, int id, int index, const std::vector<T>& value) = 0;
};
