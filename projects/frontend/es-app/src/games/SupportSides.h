//
// Created by bkg2k on 23/07/24.
//
#pragma once

enum class SupportSides : unsigned char
{
  None, //!< No face
  A,    //!< Face A
  B,    //!< Face B
};

class String;

String SupportSideToString(SupportSides side);