//
// Created by bkg2k on 14/11/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/String.h>
#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>
#include <utils/os/system/Mutex.h>
#include <cassert>

class StringHolderTypes
{
  public:
    //! 32bits string index
    typedef int Index32;
    //! 16bits string index
    typedef short Index16;
    //! 8bits string index
    typedef char Index8;

    //! Fast search result structure
    struct IndexAndDistance
    {
      Index32 Index;    //!< Index
      short Distance; //!< Distance to the text found, in characters
      short Context;  //!< User context
    };

    //! Search results
    typedef Array<IndexAndDistance> FoundTextList;
};

template< int bits>
class MetadataStringHolder
{
  public:
    /*!
     * @brief Constructor
     * @param capacity Initial capacity
     * @param granularity Granularity
     */
    MetadataStringHolder(int capacity, int granularity)
      : mMetaString(capacity, granularity)
      , mIndexes(1024, 1024)
    {
      AddString32(String());
    }

    /*!
     * @brief Add a string and return its index in 32bit format
     * @param newString String to add
     * @return String index
     */
    StringHolderTypes::Index32 AddString32(const String& newString)
    {
      // Synchronize
      Mutex::AutoLock locker(mSyncher);

      // Already exists?
      StringHolderTypes::Index32 result = 0;
      if (FetchOrAdd(newString, result)) return result;

      // Add string
      mIndexes.Add(mMetaString.Count());
      const char* stringData = newString.data();
      int stringLength = newString.Count();
      mMetaString.AddItems(stringData != nullptr ? stringData : "", stringLength + 1); // Include zero terminal

      return result;
    }

    /*!
     * @brief Add a string and return its index in 32bit format
     * @param newString String to add
     * @return String index
     */
    StringHolderTypes::Index16 AddString16(const String& newString) { StringHolderTypes::Index32 index = AddString32(newString); assert((index >> 16) == 0); return (StringHolderTypes::Index16)index; }

    /*!
     * @brief Add a string and return its index in 32bit format
     * @param newString String to add
     * @return String index
     */
    StringHolderTypes::Index8 AddString8(const String& newString) { StringHolderTypes::Index32 index = AddString32(newString); assert((index >> 8) == 0); return (StringHolderTypes::Index8)index; }


    /*!
     * @brief Get a string from its index
     * @param index String index
     * @return String
     */
    String GetString(StringHolderTypes::Index32 index)
    {
      // Synchronize
      Mutex::AutoLock locker(mSyncher);

      // Out of range
      if ((unsigned int)index >= (unsigned int)mIndexes.Count())
        #ifdef DEBUG
        return String("Unknown Index ").Append(index);
      #else
      {
        { LOG(LogError) << "[MetadataStringHolder] GetString - Index error! " << index; }
        return String();
      }
      #endif
      // Regular string
      if (index < mIndexes.Count() - 1) return String(&mMetaString(mIndexes[index]), mIndexes[index + 1] - 1 - mIndexes[index]);
      // Last item is zero terminal
      return String(&mMetaString(mIndexes[index]), mMetaString.Count() - 1 - mIndexes[index]);
    }

    /*!
     * @brief Get a string from its index
     * @param index String index
     * @return String
     */
    String GetString(StringHolderTypes::Index32 index, const String& defaultValue)
    {
      String result = GetString(index);
      if (result.empty()) return defaultValue;
      return result;
    }

    /*!
     * @brief Get a path from the string at the given index
     * @param index String index
     * @return Path
     */
    Path GetPath(StringHolderTypes::Index32 index)
    {
      // Synchronize
      Mutex::AutoLock locker(mSyncher);

      // Out of range
      if ((unsigned int)index >= (unsigned int)mIndexes.Count())
        #ifdef DEBUG
        return Path("/unknown/index") / String(index);
      #else
      {
        { LOG(LogError) << "[MetadataStringHolder] GetPath - Index error! " << index; }
        return Path::Empty;
      }
      #endif
      // Regular string
      if (index < mIndexes.Count() - 1) return Path(&mMetaString(mIndexes[index]), mIndexes[index + 1] - 1 - mIndexes[index]);
      // Last item is zero terminal
      return Path(&mMetaString(mIndexes[index]), mMetaString.Count() - 1 - mIndexes[index]);
    }

    //! Get storage size
    [[nodiscard]] int StorageSize() const
    {
      int result = 0;
      for(int i = sSplitSize; --i >= 0; ) result += sizeof(Item) * mArray[i].Count();
      return result + mMetaString.Capacity() + mIndexes.ByteSize();
    }

    //! Get storage size
    [[nodiscard]] int ObjectCount() const { return mIndexes.Count(); }

    //! Is the given index valid?
    [[nodiscard]] bool IsValid(int index) const { return index < mIndexes.Count(); }

    /*!
     * @brief Search text and return a list of index (and distance for sorting)
     * @param text Text to search for
     * @param output Output list to fill with found results
     */
    void FindText(const String& text, StringHolderTypes::FoundTextList& output, int context)
    {
      const char* fdn = mMetaString.BufferReadOnly();             // Keep filedata name pointer for fast result computation
      const char* tts = text.c_str();                             // Keep text pointer for fast search reset
      int lmax = mMetaString.Count() - (int)text.size() + 1;      // Maximum byte to search in

      for(const char* p = fdn; --lmax >= 0; ++p)                  // Run through the game name, straight forward
        if ((*p | 0x20) == (*tts))                                // Try to catch the first char
          for (const char* s = tts, *ip = p; ; )                  // Got it, run through both string
          {
            const char c = *(++s);
            if ((c != 0) && ((*(++ip) | 0x20) != c) ) break;      // Chars are not equal, exit the inner loop
            if (c == 0)
            {
              output.Add(IndexFromPos((int) (p - fdn), context)); // Chars are equal, got a zero terminal? Found it!
              break;
            }
          }
    }

  private:
    //! Synchronizer
    Mutex mSyncher;
    //! String containing all substrings
    Array<char> mMetaString;
    //! Substring indexes to char indexes
    Array<StringHolderTypes::Index32> mIndexes;

    //! Real size of 1st level static array
    static constexpr int sSplitSize = (1 << bits);
    //! Mask of 1rst level splitter
    static constexpr int sSplitMask = sSplitSize - 1;

    //! Stored structure
    struct Item
    {
      int mHash;  //!< String hash
      int mIndex; //!< String index
      int mSize;  //!< String size
    } __attribute__((packed));

    //! 1rst level array of second level arrays !
    Array<Item> mArray[sSplitSize];

    /*!
     * @brief Try to get the string index if it already exists, or add the string at the given index
     * @param string String whose index is to be retrieved
     * @param newIndex New index to associate with the string if the string is not found
     * @return If the string is found, the returned value is an index different from newIndex
     * if the string is not found, the string has been added and the returned value is newIndex.
     */
    bool FetchOrAdd(const String& string, [[out]] StringHolderTypes::Index32& result)
    {
      int hash = string.Hash();

      // Lookup value
      Array<Item>& array = mArray[hash & sSplitMask];
      for(int count = (array.Count() + 1) / 2, b = -1, t = array.Count(); --count >= 0;)
      {
        if (Item& item = array(++b); item.mHash == hash)
          if (item.mSize == string.Count())
            if (memcmp(string.data(), &mMetaString(mIndexes[item.mIndex]), item.mSize ) == 0)
            {
              result = item.mIndex;
              return true;
            }
        if (Item& item = array(--t); item.mHash == hash)
          if (item.mSize == string.Count())
            if (memcmp(string.data(), &mMetaString(mIndexes[item.mIndex]), item.mSize ) == 0)
            {
              result = item.mIndex;
              return true;
            }
      }

      result = mIndexes.Count();
      array.Add(Item(hash, result, string.Count()));
      return false;
    }

    /*!
     * @brief Get the index of the sub-string at the given char position from metastring
     * @param pos Char position in the metastring
     * @return Index or -1 if the position is OOB
     */
    StringHolderTypes::IndexAndDistance IndexFromPos(int pos, int context)
    {
      // Out of bounds?
      if ((unsigned int)pos > (unsigned int)mMetaString.Count()) return { -1, -1, -1 };
      // Last item? (avoid the end computation in the main loop)
      if (pos >= mIndexes.Last()) return { mIndexes.Count() - 1, (short)(pos - mIndexes.Last()), (short)context };

      for(int left = 0, right = mIndexes.Count() - 1; ; )
      {
        int pivot = (left + right) >> 1;
        if (pos < mIndexes[pivot]) right = pivot;
        else if (pos >= mIndexes[pivot + 1]) left = pivot + 1;
        else return { .Index=pivot, .Distance=(short)(pos - mIndexes[pivot]), .Context=(short)context };
      }
    }

};



