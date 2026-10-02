//
// Created by bkg2k on 30/06/26.
//
#pragma once

#include "ThemePropertyBag.h"
#include <utils/storage/Array.h>
#include <utils/os/system/Mutex.h>

/*!
 * @brief Process-wide deduplicating pool of PropertyBag values.
 *
 * A single instance lives in ThemeManager. Since PropertyBag is a 16-byte POD referencing its
 * string by index, identical values (same color, same position, same font index...) collapse to a
 * single stored instance shared by every element of every system. Theme elements then reference a
 * property value by a 32-bit index instead of embedding a full PropertyBag.
 *
 * Theme loading is multi-threaded, so reads and writes are serialized by a mutex; Get() returns a
 * copy (under the lock) so no reference into the arena can dangle during a concurrent Intern().
 */
class ThemePropertyBagPool
{
  public:
    ThemePropertyBagPool() { Reset(); }

    //! Drop all content (called on theme reload, before re-parsing)
    void Reset()
    {
      Mutex::AutoLock locker(mLock);
      mBags.Clear();
      mNext.Clear();
      mHeads.Clear();
      for (int i = sBuckets; --i >= 0;) mHeads.Add(-1);
    }

    //! Intern a value, reusing the existing index if an equal one already exists (thread-safe)
    int Intern(const PropertyBag& bag)
    {
      Mutex::AutoLock locker(mLock);
      const int bucket = (int)(bag.Hash() & (unsigned int)sMask);
      for (int i = mHeads[bucket]; i >= 0; i = mNext[i])
        if (mBags(i) == bag) return i;
      const int index = mBags.Count();
      mBags.Add(bag);
      mNext.Add(mHeads[bucket]);
      mHeads(bucket) = index;
      return index;
    }

    //! Get a copy of the value at the given index (thread-safe)
    [[nodiscard]] PropertyBag Get(int index) const
    {
      Mutex::AutoLock locker(mLock);
      return mBags[index];
    }

  private:
    //! Head table size (power of two)
    static constexpr int sBuckets = 4096;
    static constexpr int sMask = sBuckets - 1;

    mutable Mutex mLock;
    Array<PropertyBag> mBags;  //!< Unique values
    Array<int> mNext;          //!< value index -> next index in the same hash bucket (-1 = end)
    Array<int> mHeads;         //!< hash bucket -> first value index (-1 = empty)
};
