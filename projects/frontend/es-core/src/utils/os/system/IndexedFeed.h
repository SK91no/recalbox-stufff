//
// Created by bkg2k on 30/07/25.
//
#pragma once

/*!
 * @brief Indexed feed (e.g: Feed + injection index)
 */
template<class FeedObject> struct IndexedFeed
{
  public:
    int        Index;    //!< Index
    int        Priority; //!< Priority - Lowest priority are processed first
    FeedObject Feed;     //!< Source object

    /*!
     * @brief Constructor
     * @param index Index
     * @param feed Result
     */
    IndexedFeed(int index, FeedObject feed, int priority = 0)
      : Index(index),
        Priority(priority),
        Feed(feed)
    {
    }

    IndexedFeed()
      : Index(0),
        Priority(0),
        Feed()
    {
    }
};