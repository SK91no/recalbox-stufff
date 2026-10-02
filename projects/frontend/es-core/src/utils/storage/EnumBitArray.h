//
// Created by bkg2k on 17/12/23.
//
#pragma once

#include <concepts>
#include <initializer_list>

//! Simple concept of a enum class
template<class T>
concept IsEnum = (std::is_enum_v<T>);

template<IsEnum T, T maxValue> class EnumBitArray
{
  public:
    //! Constructor
    EnumBitArray()
      : mBits()
    {
      memset(mBits, 0, sizeof(mBits));
    }

    EnumBitArray(std::initializer_list<T> il)
      : EnumBitArray()
    {
      for(T item : il) Set(item);
    }

    //! Set value
    void Set(T value)
    {
      mBits[(int)value >> sBitShift] |= 1ULL << ((int)value & sBitMask);
    }

    //! Unset value
    void Unset(T value)
    {
      mBits[(int)value >> sBitShift] &= ~1ULL << ((int)value & sBitMask);
    }

    //! Set or unset value regarding the given state
    void Set(T value, bool state)
    {
      if (state) Set(value);
      else Unset(value);
    }

    //! Check value state
    bool IsSet(T value) const
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
    StorageType mBits[((int)maxValue + sBitMask) >> sBitShift];
};
