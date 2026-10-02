//
// Created by bkg2k on 30/06/26.
//
#pragma once

#include <utils/String.h>
#include <utils/storage/Array.h>
#include <utils/os/system/Mutex.h>
#include <cstring>

/*!
 * @brief Process-wide deduplicating string pool for theme PropertyBag values.
 *
 * A single instance lives in ThemeManager and is shared by every ThemeData. The vast majority
 * of theme strings (font paths, image paths, hex colors stored as text, etc.) are identical from
 * one system to another, so deduplication collapses ~140 identical copies into a single one, and
 * the contiguous arena removes per-string heap blocks (alignment / malloc-chunk overhead).
 *
 * Theme loading is multi-threaded (system themes are parsed in a thread pool), so BOTH reads and
 * writes are serialized by a mutex. Get() returns a String by value (copied under the lock) so the
 * caller never holds a reference into the arena while another thread may reallocate it.
 */
class ThemeStringPool
{
  public:
    //! Index of the always-present empty string
    static constexpr int Empty = 0;

    ThemeStringPool() { Reset(); }

    //! Drop all content (called on theme reload, before re-parsing)
    void Reset()
    {
      Mutex::AutoLock locker(mLock);
      mArena.Clear();
      mOffsets.Clear();
      mNext.Clear();
      mHeads.Clear();
      for (int i = sBuckets; --i >= 0;) mHeads.Add(-1);
      InternLocked(String::Empty); // index 0
    }

    /*!
     * @brief Intern a string, reusing the existing index if it already exists (thread-safe)
     * @return Compact index usable with Get()
     */
    int Intern(const String& s)
    {
      Mutex::AutoLock locker(mLock);
      return InternLocked(s);
    }

    //! Get a copy of the string at the given index (thread-safe, copied under the lock)
    [[nodiscard]] String Get(int index) const
    {
      Mutex::AutoLock locker(mLock);
      const int start = mOffsets[index];
      const int end = (index + 1 < mOffsets.Count()) ? (mOffsets[index + 1] - 1) : (mArena.Count() - 1);
      return { mArena.BufferReadOnly() + start, end - start };
    }

  private:
    //! Head table size (power of two)
    static constexpr int sBuckets = 4096;
    static constexpr int sMask = sBuckets - 1;

    mutable Mutex mLock;
    Array<char> mArena;   //!< All strings, null-terminated, packed
    Array<int> mOffsets;  //!< string index -> byte offset in mArena
    Array<int> mNext;     //!< string index -> next index in the same hash bucket (-1 = end)
    Array<int> mHeads;    //!< hash bucket -> first string index (-1 = empty)

    //! Intern without locking (caller holds mLock)
    int InternLocked(const String& s)
    {
      const int len = s.Count();
      const char* data = s.data();
      const unsigned int hash = (unsigned int)s.Hash();
      const int bucket = (int)(hash & (unsigned int)sMask);
      for (int i = mHeads[bucket]; i >= 0; i = mNext[i])
      {
        const int start = mOffsets[i];
        const int end = (i + 1 < mOffsets.Count()) ? (mOffsets[i + 1] - 1) : (mArena.Count() - 1);
        if (end - start == len && memcmp(mArena.BufferReadOnly() + start, data, (size_t)len) == 0)
          return i;
      }
      const int index = mOffsets.Count();
      mOffsets.Add(mArena.Count());
      if (len != 0) mArena.AddItems(data, len);
      mArena.Add('\0');
      mNext.Add(mHeads[bucket]);
      mHeads(bucket) = index;
      return index;
    }
};
