//
// Created by bkg2k on 29/06/26.
//
#pragma once

#include <utils/String.h>
#include <utils/storage/Array.h>
#include <cstring>

/*!
 * @brief Append-only, deduplicating string arena for gamelist display names.
 *
 * Display names are stored once and referenced by a compact 32-bit index, instead
 * of keeping a full std::string (32 bytes + heap block) per list entry.
 * Identical names are interned only once: this is very profitable on computer-like
 * systems where the same title ships on multiple supports (disks/tapes) and therefore
 * appears many times with the exact same name.
 *
 * Deduplication uses a chained hash table (a single head table + per-name next links)
 * so lookup stays O(1) amortized while keeping the structural overhead low.
 *
 * Not thread-safe on purpose: gamelists are built and rendered on the UI thread only.
 */
class GamelistNameArena
{
  public:
    //! Index of the always-present empty string
    static constexpr int Empty = 0;

    GamelistNameArena() { Reset(0); }

    /*!
     * @brief Drop all content and pre-size the hash table for the expected name count.
     * @param expectedNames Expected number of (possibly duplicated) names to come
     */
    void Reset(int expectedNames)
    {
      mArena.Clear();
      mOffsets.Clear();
      mNext.Clear();
      // Size the head table to the next power of two >= ~1.5x the expected count, capped.
      int wanted = expectedNames + (expectedNames >> 1) + 1;
      int size = sMinBuckets;
      while (size < wanted && size < sMaxBuckets) size <<= 1;
      mMask = size - 1;
      mHeads.Clear();
      for (int i = size; --i >= 0;) mHeads.Add(-1);
      // Index 0 is always the empty string
      Intern(String::Empty);
    }

    /*!
     * @brief Intern a name, reusing the existing index if the exact same name already exists.
     * @param name Name to intern
     * @return Compact index usable with Get()/CString()/Length()
     */
    int Intern(const String& name)
    {
      const int length = name.Count();
      const char* data = name.data();
      const unsigned int hash = (unsigned int)name.Hash();
      const int bucket = (int)(hash & (unsigned int)mMask);
      // Lookup an identical name in the bucket chain
      for (int i = mHeads[bucket]; i >= 0; i = mNext[i])
        if (Length(i) == length && memcmp(CString(i), data, (size_t)length) == 0)
          return i;
      // Not found: append the name (null-terminated) and chain it into the bucket
      const int index = mOffsets.Count();
      mOffsets.Add(mArena.Count());
      if (length != 0) mArena.AddItems(data, length);
      mArena.Add('\0');
      mNext.Add(mHeads[bucket]);
      mHeads(bucket) = index;
      return index;
    }

    //! Get a null-terminated C-string view (zero allocation), valid until the next Reset()
    [[nodiscard]] const char* CString(int index) const { return mArena.BufferReadOnly() + mOffsets[index]; }

    //! Get a String copy of the name at the given index (allocates)
    [[nodiscard]] String Get(int index) const { return { CString(index), Length(index) }; }

    //! Byte length of the name at the given index
    [[nodiscard]] int Length(int index) const
    {
      const int start = mOffsets[index];
      const int end = (index + 1 < mOffsets.Count()) ? (mOffsets[index + 1] - 1) : (mArena.Count() - 1);
      return end - start;
    }

  private:
    //! Minimum head table size
    static constexpr int sMinBuckets = 64;
    //! Maximum head table size (keeps the structural overhead bounded on huge systems)
    static constexpr int sMaxBuckets = 4096;

    //! All names, null-terminated, packed back to back
    Array<char> mArena;
    //! Name index -> byte offset in mArena
    Array<int> mOffsets;
    //! Name index -> next name index in the same hash bucket (-1 = end of chain)
    Array<int> mNext;
    //! Hash bucket -> first name index (-1 = empty bucket)
    Array<int> mHeads;
    //! Bucket mask (head table size - 1)
    int mMask = 0;
};
