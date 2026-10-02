//
// Created by bkg2k on 13/12/2020.
//
#pragma once

#include <rendering/fonts/Glyph.h>
#include <utils/storage/HashMap.h>

//! Unicode representation
typedef unsigned int UnicodeChar;

/*!
 * @brief This class holds GlyphCollection
 * The collection must guarantee that Glyph instance are never moved
 * so that references accross API calls are always valid
 */
class GlyphStorage
{
  public:
    //! Default constructor
    GlyphStorage()
      : mIndex(0)
    {
    }

    //! Destructor
    ~GlyphStorage()
    {
      CleanUp();
    }

    //! Clean up and free all glyph
    void CleanUp()
    {
      // Clear hashmap
      mGlyphCollection.clear();
      // Clear arrays
      for(int i = mGlyphStore.Count(); --i >= 0; )
        delete[] mGlyphStore[i];
      // Clear array list
      mGlyphStore.Clear();
      mIndex = 0;
    }

    //! Allocate a new empty Glyph
    Glyph* New(UnicodeChar character)
    {
      // Need new array
      if (mIndex >= sGlyphArraySize || mGlyphStore.Empty())
      {
        Glyph* newArray = new Glyph[sGlyphArraySize];
        mGlyphStore.Add(newArray);
        mIndex = 0;
      }
      Glyph* glyph = &(mGlyphStore.Last()[mIndex++]);
      mGlyphCollection[character] = glyph;
      return glyph;
    }

    Glyph* Get(UnicodeChar character)
    {
      Glyph** result = mGlyphCollection.try_get(character);
      if (result != nullptr) return *result;
      return nullptr;
    }

  private:
    //! Glyph array size - Number of glyph allocated at once
    static constexpr int sGlyphArraySize = 128;

    //! Non moving Glyph storage
    Array<Glyph*> mGlyphStore;

    //! Uncode to Glyph direct access
    HashMap<UnicodeChar, Glyph*> mGlyphCollection;

    //! Next free glyph index
    int mIndex;
};
