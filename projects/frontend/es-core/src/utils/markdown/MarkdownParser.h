//
// Created by bkg2k on 31/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <string>
#include <vector>
#include <utils/storage/Array.h>
#include "MarkdownItem.h"

class MarkdownParser
{
  public:
    MarkdownParser() = default;

    explicit MarkdownParser(const String& markdownText)
      : mTokens(32)
    {
      Parse(markdownText);
    }

    void Parse(const String& markdownText);

    [[nodiscard]] int ItemCount() const { return (int)mItems.size(); }

    [[nodiscard]] const MarkdownItem& Item(int index) const { return (unsigned int)index < mItems.size() ? mItems[index] : sNullItem; }

  private:
    //! Height byte structure
    struct Token
    {
      unsigned short StartOffset;  //!< Start char (including marker)
      unsigned short StopOffset;   //!< Total length (including marker)
      MarkdownItem::FontType Type; //!< Type of font change
      char Marker;                 //!< Marker type (avoir */_ mixes)
      char MarkerSize;             //!< Marker size (1 or 2 chars)

      [[nodiscard]] bool Contains(const Token& token) const
      {
        return token.StartOffset > StartOffset &&
               token.StopOffset < StopOffset;
      }
    } __attribute__((packed));

    struct Indexes // Parsing indexes
    {
      int LastOpenBold;
      int LastOpenItalicStar;
      int LastOpenItalicUnder;
      int LastOpenStrike;
      int LastOpenUnderline;

      Indexes()
        : LastOpenBold(-1)
        , LastOpenItalicStar(-1)
        , LastOpenItalicUnder(-1)
        , LastOpenStrike(-1)
        , LastOpenUnderline(-1)
      {}

      void CloseBold()
      {
        if (LastOpenItalicStar > LastOpenBold) LastOpenItalicStar = -1;
        if (LastOpenItalicUnder > LastOpenBold) LastOpenItalicUnder = -1;
        if (LastOpenStrike > LastOpenBold) LastOpenStrike = -1;
        if (LastOpenUnderline > LastOpenBold) LastOpenUnderline = -1;
        LastOpenBold = -1;
      }

      void CloseItalicStar()
      {
        if (LastOpenBold > LastOpenItalicStar) LastOpenBold = -1;
        if (LastOpenItalicUnder > LastOpenItalicStar) LastOpenItalicUnder = -1;
        if (LastOpenStrike > LastOpenItalicStar) LastOpenStrike = -1;
        if (LastOpenUnderline > LastOpenItalicStar) LastOpenUnderline = -1;
        LastOpenItalicStar = -1;
      }

      void CloseItalicUnder()
      {
        if (LastOpenBold > LastOpenItalicUnder) LastOpenBold = -1;
        if (LastOpenItalicStar > LastOpenItalicUnder) LastOpenItalicStar = -1;
        if (LastOpenStrike > LastOpenItalicUnder) LastOpenStrike = -1;
        if (LastOpenUnderline > LastOpenItalicUnder) LastOpenUnderline = -1;
        LastOpenItalicUnder = -1;
      }

      void CloseStrikeThru()
      {
        if (LastOpenBold > LastOpenStrike) LastOpenBold = -1;
        if (LastOpenItalicStar > LastOpenStrike) LastOpenItalicStar = -1;
        if (LastOpenItalicUnder > LastOpenStrike) LastOpenItalicUnder = -1;
        if (LastOpenUnderline > LastOpenStrike) LastOpenUnderline = -1;
        LastOpenStrike = -1;
      }

      void CloseUnderline()
      {
        if (LastOpenBold > LastOpenUnderline) LastOpenBold = -1;
        if (LastOpenItalicStar > LastOpenUnderline) LastOpenItalicStar = -1;
        if (LastOpenItalicUnder > LastOpenUnderline) LastOpenItalicUnder = -1;
        if (LastOpenStrike > LastOpenUnderline) LastOpenStrike = -1;
        LastOpenUnderline = -1;
      }
    };

    static const MarkdownItem sNullItem;

    static const Token sNullToken;

    std::vector<MarkdownItem> mItems;

    // Reserve a 1024 token list
    Array<Token> mTokens;

    /*
     * Parser
     */

    static MarkdownItem::FontSize ParseHeader(const char* line, int& start, int& end);

    static MarkdownItem::Decorations ParseBullet(const char* line, int& start, int end);

    static MarkdownItem::Decorations ParseNumberedBullets(const char* line, int& start, int end, int& number);

    bool ParseSeparator(const char* line, int start, int end);

    void ParseText(const char* line, int start, int end, MarkdownItem::Decorations decoration, MarkdownItem::FontSize fontSize, int number);

    void ParserLine(const char* line, int start, int end);

    int ParseAntiQuote(const char* line, int start, int end);

    /*
     * Tokens
     */

    void RecordItalicUnderlineTriUnderscore(Indexes& indexes, int start);

    void RecordUnderlineUnderscore(Indexes& indexes, int start);

    void RecordItalicUnderscore(Indexes& indexes, int start);

    void RecordStrikeThruTilda(Indexes& indexes, int start);

    void RecordBoldItalicTristar(Indexes& indexes, int start);

    void RecordBoldStar(Indexes& indexes, int start);

    void RecordItalicStar(Indexes& indexes, int start);

    /*
     * Items
     */

    MarkdownItem& New();

    MarkdownItem& Previous();

    int LastLineEnd(int from);

    int LastLineStart(int from);

    void EmptyLine();

    int TokensToItems(int index, const char* line, MarkdownItem::Decorations decorations, MarkdownItem::FontSize size, MarkdownItem::FontType type, int number);

    void SplitItems(const char* line, int start, int stop, int number, MarkdownItem::Decorations decorations, MarkdownItem::FontSize size, MarkdownItem::FontType type);
};



