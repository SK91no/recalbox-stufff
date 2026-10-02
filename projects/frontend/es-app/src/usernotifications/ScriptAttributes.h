//
// Created by bkg2k on 30/05/24.
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

enum class ScriptAttributes
{
  None           = 0, //!< Nothing
  Permanent      = 1, //!< Script runs permanently
  Synced         = 2, //!< The frontend wait for execution. Mutually exclusive with Permanent
  ReportProgress = 8, //!< If this script is manual, it report progress, either %age of progress or raw text
};

DEFINE_BITFLAG_ENUM(ScriptAttributes, int)