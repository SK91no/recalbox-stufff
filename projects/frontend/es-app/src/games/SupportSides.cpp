//
// Created by bkg2k on 02/06/25.
//

#include "SupportSides.h"
#include "utils/String.h"

String SupportSideToString(SupportSides side)
{
  switch(side)
  {
    case SupportSides::A: { static String s("A"); return s; }
    case SupportSides::B: { static String s("B"); return s; }
    case SupportSides::None:
    default: break;
  }

  static String s("N/A");
  return s;
}