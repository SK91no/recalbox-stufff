//
// Created by bkg2k on 31/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/cplusplus/Bitflags.h>

class MarkdownItem
{
  public:
    enum class Decorations : unsigned char
    {
      None           = 0,
      Bullet         = 1,  //!< Bullet list entry before the current text
      Numbered       = 2,  //!< Numbered list entry before the current text
      NewLine        = 4,  //!< New line after the current text
      EmptyLine      = 8,  //!< Empty line after the current text
      HorizontalRule = 16, //!< Horizontal rule after the current text
    };

    enum class FontSize : unsigned char
    {
      Normal = 0, //!< Normal text
      H1     = 1, //!< Heading 1
      H2     = 2, //!< Heading 2
      H3     = 3, //!< Heading 3
      H4     = 4, //!< Heading 4
    };

    enum class FontType : unsigned char
    {
      None       = 0,
      Bold       = 1,
      Italic     = 2,
      StrikeThru = 4,
      Underline  = 8,
      Fixed      = 16,
    };

    //! Get text start index
    [[nodiscard]] int TextStartIndex() const { return mTextStart; }
    //! Get text e,d index
    [[nodiscard]] int TextEndIndex() const { return mTextEnd; }

    //! Get header type. 0 = normal text, X = header X
    [[nodiscard]] int Header() const { return (int)mFontSize; }

    //! Check if the current text needs to be preceded by a right tabulation + a bullet
    [[nodiscard]] bool HasBullet() const { return ((int)mDecoration & (int)Decorations::Bullet) != 0; }
    //! Check if the current text needs to be preceded by a right tabulation + a number
    [[nodiscard]] bool HasNumbered() const { return ((int)mDecoration & (int)Decorations::Numbered) != 0; }
    //! Check if the current text needs a new line after
    [[nodiscard]] bool HasNewLine() const { return ((int)mDecoration & (int)Decorations::NewLine) != 0; }
    //! Check if the current text needs an empty line after
    [[nodiscard]] bool HasEmptyLine() const { return ((int)mDecoration & (int)Decorations::EmptyLine) != 0; }
    //! Check if the current text needs a new line + horizontal rule + new line
    [[nodiscard]] bool HasHorizontalRule() const { return ((int)mDecoration & (int)Decorations::HorizontalRule) != 0; }

    //! Check if the current text must be rendered bold
    [[nodiscard]] bool IsBold() const { return ((int)mFontType & (int)FontType::Bold) != 0; }
    //! Check if the current text must be rendered italic
    [[nodiscard]] bool IsItalic() const { return ((int)mFontType & (int)FontType::Italic) != 0; }
    //! Check if the current text must be struck thru
    [[nodiscard]] bool IsStrikeTrhu() const { return ((int)mFontType & (int)FontType::StrikeThru) != 0; }
    //! Check if the current text must be fixed font
    [[nodiscard]] bool IsFixed() const { return ((int)mFontType & (int)FontType::Fixed) != 0; }
    //! Check if the current text must be underline
    [[nodiscard]] bool IsUnderline() const { return ((int)mFontType & (int)FontType::Underline) != 0; }

    [[nodiscard]] int Number() const { return mNumber; }

  private:
    // Allow Parser to access private member
    friend class MarkdownParser;

    //! Private constructor
    MarkdownItem(int start, int end, unsigned char number, Decorations decorations, FontSize fontsize, FontType fonttype)
      : mTextStart(start)
      , mTextEnd(end)
      , mNumber(number)
      , mDecoration(decorations)
      , mFontSize(fontsize)
      , mFontType(fonttype)
    {
    }

    //! Start position of this text chunk in the original text string
    int mTextStart;
    //! Stop (inclusive) position of this text chunk in the original text string
    int mTextEnd;
    //! Numbered bullet number
    unsigned char mNumber;
    //! Décorations
    Decorations mDecoration;
    //! Font size
    FontSize mFontSize;
    //! Font type
    FontType mFontType;
};

DEFINE_BITFLAG_ENUM(MarkdownItem::Decorations, unsigned char)
DEFINE_BITFLAG_ENUM(MarkdownItem::FontType, unsigned char)

