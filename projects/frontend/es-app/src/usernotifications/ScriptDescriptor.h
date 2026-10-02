//
// Created by bkg2k on 06/12/24.
//
#pragma once

#include "ScriptAttributes.h"
#include <utils/os/fs/Path.h>

/*!
 * @brief Script descriptor
 */
struct ScriptDescriptor
{
  Path             mPath;      //!< Script path
  int              mIndex;     //! internal index required for manual script execution
  ScriptAttributes mAttribute; //!< Script attributes
};

//! Script descriptor list
typedef std::vector<ScriptDescriptor> ScriptDescriptorList;
