//
// Created by bkg2k on 23/10/24.
//
#pragma once

#include "Array.h"

//! Bit class array w/ auto expansion
class BitArray
{
  public:
    //! Constructor
    BitArray()
    {
    }

    //! Constructor
    BitArray(int max)
    {
      mBits.ExpandTo((max + sBitCount - 1) / sBitCount);
    }

    //! Unset all bits
    void UnsetAll() { memset(mBits.ByteBufferReadWrite(), 0, mBits.ByteSize()); }

    //! Set all bits
    void SetAll() { memset(mBits.ByteBufferReadWrite(), 0xFF, mBits.ByteSize()); }

    //! Set value
    void Set(int value)
    {
      mBits((int)value >> sBitShift) |= 1ULL << ((int)value & sBitMask);
    }

    //! Unset value
    void Unset(int value)
    {
      mBits((int)value >> sBitShift) &= ~1ULL << ((int)value & sBitMask);
    }

    //! Set or unset value regarding the given state
    void Set(int value, bool state)
    {
      if (state) Set(value);
      else Unset(value);
    }

    //! Check value state
    bool IsSet(int value) const
    {
      return ((mBits[(int)value >> sBitShift] >> ((int)value & sBitMask)) & 1) != 0;
    }

  private:
    //! Underlying type
    typedef unsigned long long StorageType;
    //! Size of underlying types, in bits
    static constexpr int sBitCount = 64;
    //! Mask of underlying type
    static constexpr int sBitMask = sBitCount - 1;
    //! Underlying type shift
    static constexpr int sBitShift = 6;
    //! Array of bits
    Array<StorageType> mBits;
};
