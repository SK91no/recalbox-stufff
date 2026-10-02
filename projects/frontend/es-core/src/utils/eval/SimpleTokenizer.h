//
// Created by bkg2k on 16/01/24.
//
#pragma once

#include "utils/String.h"

class SimpleTokenizer
{
  public:
    enum class Type
    {
      Start,      //!< Just started, no token parsed yet
      Identifier, //!< Variable
      And,        //!< AND condition
      Or,         //!< Or condition
      Not,        //!< Not unary
      Open,       //!< Opening sub-expression (
      Close,      //!< Closing sub-expression )
      Error,      //!< Unreconized token, syntax error !
      End,        //!< No more tokens
    };

    /*!
     * @brief Tokenize a string
     * @param string String to tokenize
     */
    explicit SimpleTokenizer(const String& string)
      : mString(string.ToLowerCase())
      , mIndex(0)
      , mType(Type::Start)
    {
    }

    //! Parse next token and return its type
    Type Next();

    //! Get last token parsed
    [[nodiscard]] const String& Token() const { return mToken; }

    //! Get type of last token parsed
    [[nodiscard]] Type TokenType() const { return mType; }

    //! Get index
    [[nodiscard]] int Index() const { return mIndex; }

    //! Get parsed string
    [[nodiscard]] const String& ParsedString() const { return mString; }

    //! Last token is an unary operator? (not)
    [[nodiscard]] bool Unary() const { return mType == Type::Not; }

  private:
    //! String to tokenize
    const String mString;
    //! Current indes in the string
    int mIndex;
    //! Last token
    String mToken;
    //! Last token type
    Type mType;
};

