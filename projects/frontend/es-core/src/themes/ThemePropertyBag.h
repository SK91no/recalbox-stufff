//
// Created by bkg2k on 30/06/26.
//
#pragma once

#include <cstring>
#include <utils/String.h>
#include <utils/os/fs/Path.h>
#include <utils/math/Vector2f.h>
#include "ThemeContexts.h"
#include "ThemeStringPool.h"
#include "ThemeVariablesHolder.h"

/*!
 * @brief Plain-old-data holder for a single theme property value.
 *
 * Strings are not stored here: they live in the shared ThemeStringPool and are referenced by a
 * 32-bit index. The result is a 16-byte POD with value semantics, which makes it trivially
 * deduplicable in a global pool (see ThemePropertyBagPool) - the same value (same color, same font
 * index, same position...) is therefore stored only once for the whole theme.
 */
class PropertyBag
{
  public:
    //! Property value kind
    enum class Type : char
    {
      Vector,     //!< Two floats
      String,     //!< Index into the string pool
      Integer,    //!< Integer
      Float,      //!< Single float
      Bool,       //!< Boolean
      Unresolved, //!< String pool index, still containing unresolved variables
    };

    //! Convert "x y" normalized pair text into two floats
    static bool ConvertPair(const String& string, float& x, float& y);

    //! Default = empty string
    PropertyBag() : mStringIndex(0), mInteger(0), mSecondFloat(0.0f), mType(Type::String), mContext(ThemeContexts::None) {}

    static PropertyBag MakeVector(float x, float y) { PropertyBag b; b.mFloat = x; b.mSecondFloat = y; b.mType = Type::Vector;  return b; }
    static PropertyBag MakeInt(int v)               { PropertyBag b; b.mInteger = v;                    b.mType = Type::Integer; return b; }
    static PropertyBag MakeFloat(float v)           { PropertyBag b; b.mFloat = v;                      b.mType = Type::Float;   return b; }
    static PropertyBag MakeBool(bool v)             { PropertyBag b; b.mBool = v;                       b.mType = Type::Bool;    return b; }
    static PropertyBag MakeString(int strIndex)     { PropertyBag b; b.mStringIndex = strIndex;         b.mType = Type::String;  return b; }
    static PropertyBag MakeUnresolved(int strIndex, ThemeContexts context) { PropertyBag b; b.mStringIndex = strIndex; b.mType = Type::Unresolved; b.mContext = context; return b; }

    //! Value equality (for deduplication) - compares fields, not raw bytes (avoids padding garbage)
    bool operator==(const PropertyBag& o) const
    {
      return mStringIndex == o.mStringIndex && mInteger == o.mInteger
          && mSecondFloat == o.mSecondFloat && mType == o.mType && mContext == o.mContext;
    }

    //! Hash (for deduplication) - mixes all significant fields
    [[nodiscard]] unsigned int Hash() const
    {
      unsigned int h = (unsigned int)mStringIndex * 2654435761u;
      h ^= (unsigned int)mInteger + 0x9e3779b9u + (h << 6) + (h >> 2);
      unsigned int sf; memcpy(&sf, &mSecondFloat, sizeof(sf));
      h ^= sf + 0x9e3779b9u + (h << 6) + (h >> 2);
      h ^= ((unsigned int)mType << 3) ^ ((unsigned int)mContext << 11);
      return h;
    }

    [[nodiscard]] String AsString(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::Vector: return String(mFloat, 4).Append(' ').Append(mSecondFloat, 4);
        case Type::String: return strings.Get(mStringIndex);
        case Type::Integer: return String(mInteger);
        case Type::Float: return String(mFloat, 4);
        case Type::Bool: return String(mBool ? '1' : '0');
        case Type::Unresolved: { String s = strings.Get(mStringIndex); variables.Resolve(s, mContext); return s; }
      }
      return String();
    }
    [[nodiscard]] Path AsPath(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::Vector: return Path(String(mFloat, 4).Append(' ').Append(mSecondFloat, 4));
        case Type::String: return Path(strings.Get(mStringIndex)).ToCanonical();
        case Type::Integer: return Path(String(mInteger));
        case Type::Float: return Path(String(mFloat, 4));
        case Type::Bool: return Path(String(mBool ? '1' : '0'));
        case Type::Unresolved: { String s = strings.Get(mStringIndex); variables.Resolve(s, mContext); return Path(s); }
      }
      return Path();
    }
    [[nodiscard]] int AsInt(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::String: { String s = strings.Get(mStringIndex); int result = 0; return s.TryAsInt(result) ? result : 0; }
        case Type::Integer: return mInteger;
        case Type::Vector:
        case Type::Float: return (int)mFloat;
        case Type::Bool: return (int)mBool;
        case Type::Unresolved: { String s = strings.Get(mStringIndex); variables.Resolve(s, mContext); int result = 0; return s.TryAsInt(result) ? result : 0; }
      }
      return 0;
    }
    [[nodiscard]] float AsFloat(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::String: { String s = strings.Get(mStringIndex); float result = 0; return s.TryAsFloat(result) ? result : 0.0f; }
        case Type::Integer: return (float)mInteger;
        case Type::Vector:
        case Type::Float: return mFloat;
        case Type::Bool: return (float)mBool;
        case Type::Unresolved: { String s = strings.Get(mStringIndex); variables.Resolve(s, mContext); float result = 0; return s.TryAsFloat(result) ? result : 0; }
      }
      return 0.0f;
    }
    [[nodiscard]] bool AsBool(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::String: { String s = strings.Get(mStringIndex); bool result = false; if (s.TryAsBool(result)) return result; return false; }
        case Type::Integer: return (bool)mInteger;
        case Type::Vector:
        case Type::Float: return (bool)mFloat;
        case Type::Bool: return mBool;
        case Type::Unresolved: { String s = strings.Get(mStringIndex); variables.Resolve(s, mContext); bool result = false; return s.TryAsBool(result) ? result : false; }
      }
      return false;
    }
    [[nodiscard]] Vector2f AsVector(ThemeStringPool& strings, ThemeVariablesHolder& variables) const
    {
      switch(mType)
      {
        case Type::Vector: return { mFloat, mSecondFloat };
        case Type::String:
        {
          float x = 0;
          float y = 0;
          if (ConvertPair(strings.Get(mStringIndex), x, y))
            return { x, y };
          break;
        }
        case Type::Integer: return { (float)mInteger, (float)mInteger };
        case Type::Float: return { mFloat, mFloat };
        case Type::Bool: return { (float)mBool, (float)mBool };
        case Type::Unresolved:
        {
          String s = strings.Get(mStringIndex); variables.Resolve(s, mContext);
          float x = 0;
          float y = 0;
          if (s.TryAsFloat(0, ' ', x))
          {
            int pos = s.Find(' ');
            if (pos >= 0)
              if (s.TryAsFloat(pos + 1, 0, y))
                return { x, y };
          }
          break;
        }
      }
      return { 0.0f, 0.0f };
    }

  private:
    int mStringIndex;          //!< Index into the shared string pool (String / Unresolved types)
    union // Cardinal exclusive values
    {
      int mInteger;
      float mFloat;
      bool mBool;
    };
    float mSecondFloat;        //!< Second vector component (Vector type)
    Type mType;                //!< Value kind
    ThemeContexts mContext;    //!< Variable context (Unresolved type)
};
