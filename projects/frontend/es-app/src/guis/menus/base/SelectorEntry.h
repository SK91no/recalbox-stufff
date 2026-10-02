//
// Created by bkg2k on 25/08/24.
//
#pragma once

#include <utils/String.h>

//! Normalized entry structure for List components
template<typename T> struct SelectorEntry
{
  String mText;   //!< Displayed text
  T mValue;       //!< Linked Values
  bool mSelected; //!< Whether the entry is selected or not

  //! Constructor for multi selector
  SelectorEntry(const String& t, const T& v, bool s)
    : mText(t)
    , mValue(v)
    , mSelected(s)
  {}

  //! Constructor for single selector
  SelectorEntry(const String& t, const T& v)
    : mText(t)
    , mValue(v)
    , mSelected(false)
  {}

  // Entry list
  typedef std::vector<SelectorEntry> List;
};

