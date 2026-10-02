//
// Created by bkg2k on 29/07/23.
//
#pragma once

#include <utils/os/fs/Path.h>

// Forward declaration
class FileData;

//! Folder thumbnail max image count
static constexpr int sFoldersMaxGameImageCount = 3*3;

//! Path array shortcut
typedef Path FolderImagesPath[sFoldersMaxGameImageCount];

struct SlowDataInformation
{
  const FileData* mItem;      //!< Item the info are related to
  int mCount;                 //!< If item is a folder, contains the totale game count of this folder
  bool mHasP2k;               //!< If item is a game, this item contains the p2k status
  FolderImagesPath mPathList; //!< Image path list for folder display

  SlowDataInformation() : mItem(nullptr), mCount(0), mHasP2k(false) {}
  void Set(const FileData* item, int count, bool hasp2k, const FolderImagesPath* pathlist)
  {
    mItem = item;
    mCount = count;
    mHasP2k = hasp2k;
    if (pathlist != nullptr)
      for (int i = sFoldersMaxGameImageCount; --i >= 0;)
        mPathList[i] = (*pathlist)[i];
  }
};

