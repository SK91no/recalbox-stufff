//
// Created by digitalLumberjack on 28/08/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

/*!
 * @brief Keeps the index of the entry carrying the smallest timestamp.
 * It elects a candidate from the entries it is given and never from a reading of the clock,
 * so it stays right whatever the epoch the timestamps are expressed in.
 */
class LeastRecentlyUsed
{
  public:
    //! Returned when nothing has been submitted
    static constexpr int sNothing = -1;

    /*!
     * @brief Submit an entry
     * @param index Entry index
     * @param timestamp Entry timestamp, the smaller the older
     */
    void Consider(int index, long long int timestamp)
    {
      if (mIndex == sNothing || timestamp < mTimestamp) { mIndex = index; mTimestamp = timestamp; }
    }

    //! True if at least one entry has been submitted
    [[nodiscard]] bool Found() const { return mIndex != sNothing; }

    //! Index of the oldest entry submitted so far, or sNothing
    [[nodiscard]] int Index() const { return mIndex; }

    //! Timestamp of the oldest entry submitted so far
    [[nodiscard]] long long int Timestamp() const { return mTimestamp; }

  private:
    int mIndex = sNothing;      //!< Oldest entry index
    long long int mTimestamp = 0; //!< Oldest entry timestamp
};
