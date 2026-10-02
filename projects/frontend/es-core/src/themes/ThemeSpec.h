//
// Created by bkg2k on 07/02/25.
//
#pragma once

#include <utils/os/fs/Path.h>

struct ThemeSpec
{
  String Name;
  Path FolderPath;

  bool operator==(const ThemeSpec& right) const { return Name == right.Name && FolderPath == right.FolderPath; }
};
