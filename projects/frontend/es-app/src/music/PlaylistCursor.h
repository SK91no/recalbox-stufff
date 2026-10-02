//
// Created by digitalLumberjack on 28/08/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

/*!
 * @brief Reading head of a playlist: it knows where the next track should be taken from,
 * and which entry to hand out when the one under the head is not playable yet.
 */
class PlaylistCursor
{
  public:
    //! Returned when no entry of the playlist can be played
    static constexpr int sNoTrack = -1;

    /*!
     * @brief Tell the cursor how long the playlist is and where to start reading
     * @param trackCount Playlist length
     * @param startIndex Index to start from, any out-of-range value restarts from the beginning
     */
    void Reset(int trackCount, int startIndex)
    {
      mCount = trackCount > 0 ? trackCount : 0;
      mIndex = ((unsigned int)startIndex < (unsigned int)mCount) ? startIndex : 0;
    }

    //! Index the head stands on, or sNoTrack when the playlist is empty
    [[nodiscard]] int Index() const { return mCount > 0 ? mIndex : sNoTrack; }

    /*!
     * @brief Look for the first playable entry at or after the head, scanning the whole
     * playlist once. On success the head lands on the entry found. On failure the head does
     * not move, so that whoever fills the cache keeps working on the very entry we are waiting for.
     * @param isPlayable Predicate telling if the entry at the given index can be played right now
     * @return Index of the entry to play or sNoTrack
     */
    template<class IsPlayable> int Seek(IsPlayable isPlayable)
    {
      for(int offset = 0; offset < mCount; ++offset)
      {
        int index = (mIndex + offset) % mCount;
        if (isPlayable(index)) { mIndex = index; return index; }
      }
      return sNoTrack;
    }

    //! Step to the entry that follows the one just handed out
    void Advance() { if (mCount > 0) mIndex = (mIndex + 1) % mCount; }

  private:
    int mCount = 0; //!< Playlist length
    int mIndex = 0; //!< Head position
};
