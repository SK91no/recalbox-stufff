//
// Created by bkg2k on 27/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <scraping/scrapers/IScraperEngineStage.h>

class FileData;

/*!
 * @brief Simple synchronized message for internal scraper use
 */

struct ScrapeSeamlessMessage
{
  //! Message type
  enum class Types
  {
    GameProgress, //!< Game scraping progress message
    GlobalCount,  //!< Global remaining coutn message
  };
  //! Game reference
  FileData* mGame;
  //! Scraping stage
  IScraperEngineStage::Stage mStage;
  //! Changes
  MetadataType mChanges;
  //! Type
  Types mType;
};
