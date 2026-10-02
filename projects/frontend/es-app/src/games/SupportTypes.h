//
// Created by bkg2k on 09/03/25.
//
#pragma once

enum class SupportTypes : unsigned char
{
  Unknown        , //!< Unknown support type
  Cartridge      , //!< Cartridge
  CD             , //!< CD or DVD
  HDD            , //!< Hard drive disk
  FilesAndFolders, //!< Files/Folders (for fantasy or engines)
  Tape           , //!< Tapes
  QuickDisk      , //!< Quick disk (2.8" floppy)
  Floppy3        , //!< Amstrad/Sinclair 3" floppy
  Floppy35       , //!< 3.5" floppy
  Floppy525      , //!< 5.25" floppy
  PCB            , //!< Hardware PCB
  Microdisc      , //!< Microdisc (sinclair)
};

class String;

String SupportTypeToString(SupportTypes supporttype);