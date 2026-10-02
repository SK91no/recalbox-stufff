//
// Created by gugue_u
//


#pragma once

#include <string>

class SearchForcedOptions
{
  public:
    explicit SearchForcedOptions(String searchText, FolderData::FastSearchContext context, bool fullMatch)
      : mSearchText(searchText),
        mContext(context),
        mFullMatch(fullMatch)
    {}

    String mSearchText;
    FolderData::FastSearchContext mContext;
    bool mFullMatch;
};
