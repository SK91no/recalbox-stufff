//
// Created by bkg2k on 25/05/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#pragma once

#include <utils/markdown/MarkdownItem.h>

class NewFont;

class MarkdownFontHolder
{
  public:
    MarkdownFontHolder()
      : mNormal(nullptr)
      , mFixed(nullptr)
      , mHeader1(nullptr)
      , mHeader2(nullptr)
      , mHeader3(nullptr)
      , mHeader4(nullptr)
    {
    }

    MarkdownFontHolder(NewFont& normal, NewFont& header1, NewFont& header2, NewFont& header3, NewFont& header4, NewFont& fixed)
      : mNormal(&normal)
      , mFixed(&fixed)
      , mHeader1(&header1)
      , mHeader2(&header2)
      , mHeader3(&header3)
      , mHeader4(&header4)
    {
    }

    [[nodiscard]] bool IsValid() const { return mNormal != nullptr; }

    [[nodiscard]] NewFont& GetFrom(const MarkdownItem& item) const
    {
      if (item.IsFixed()) return *mFixed;
      switch(item.Header())
      {
        case 1: return *mHeader1;
        case 2: return *mHeader2;
        case 3: return *mHeader3;
        case 4: return *mHeader4;
        default: break;
      }
      return *mNormal;
    }

    [[nodiscard]] NewFont& Default() const { return *mNormal; }

  private:

    friend class NewFont;

    NewFont* mNormal;
    NewFont* mFixed;
    NewFont* mHeader1;
    NewFont* mHeader2;
    NewFont* mHeader3;
    NewFont* mHeader4;
};