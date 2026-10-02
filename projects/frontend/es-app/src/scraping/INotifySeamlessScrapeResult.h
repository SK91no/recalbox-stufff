//
// Created by bkg2k on 19/04/25.
//
#pragma once

class INotifySeamlessScrapeResult
{
  public:
    //! Virtual destructor
    virtual ~INotifySeamlessScrapeResult() = default;

    /*!
     * @brief Notify a game has been scraped
     * @param count Number of queued or running scrapes
     * 0 means no more scrape running and no more scape queued
     */
    virtual void ScrapeCountYetToGo(int count) = 0;
};
