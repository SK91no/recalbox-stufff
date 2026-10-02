//
// Created by bkg2k on 31/03/2021.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include <utils/storage/Array.h>
#include "MarkdownParser.h"

const MarkdownItem MarkdownParser::sNullItem(0, 0,0,
                                       MarkdownItem::Decorations::None,
                                       MarkdownItem::FontSize::Normal,
                                       MarkdownItem::FontType::None);

const MarkdownParser::Token MarkdownParser::sNullToken { 0, 0, MarkdownItem::FontType::None, 0, 0 };

void MarkdownParser::Parse(const String& markdownText)
{
  const char* s = markdownText.data();

  int startLine = 0;
  while(startLine < (int)markdownText.size())
  {
    int endLine = (int)markdownText.find('\n', startLine);
    if (endLine < 0) endLine = (int)markdownText.size() - 1;
    // Is line long enough
    if (endLine > startLine)
    {
      int start = startLine;
      int end = endLine;
      // Trim left - space & tabs
      while((start < end) && (s[start] == ' ' || s[start] == '\t')) ++start;
      // Trim right - stace & tabs & \r
      while((start < end) && (s[end] == ' ' || s[end] == '\t' || s[end] == '\r' || s[end] == '\n')) --end;

      // Line still valid?
      if (start < end) ParserLine(s, start, end);
      else EmptyLine();
    }
    else EmptyLine();
    startLine = endLine + 1;
  }

  // Remove trailing NewLine from the last item - ParserLine adds NewLine to every line
  // including the last one, which causes an extra line height in the text layout
  if (!mItems.empty())
    mItems.back().mDecoration &= ~MarkdownItem::Decorations::NewLine;
}

void MarkdownParser::ParserLine(const char* line, int start, int end)
{
  // Separator?
  if (line[start] == '-' || line[start] == '=')
    if (ParseSeparator(line, start, end))
      return;

  int number = 0;
  MarkdownItem::Decorations decoration = MarkdownItem::Decorations::None;
  MarkdownItem::FontSize fontSize = MarkdownItem::FontSize::Normal;

  char c = line[start];
  // Header?
  if (c == '#') fontSize = ParseHeader(line, start, end);
    // Bullets?
  else if (c == '-' || c == '+' || c == '*') decoration = ParseBullet(line, start, end);
    // Numbered bullets?
  else if ((unsigned int)((int)line[start] - (int)'0') <= 9) decoration = ParseNumberedBullets(line, start, end, number);

  // Parse text
  ParseText(line, start, end, decoration, fontSize, number);

  // Add newline
  if (!mItems.empty())
    mItems.back().mDecoration |= MarkdownItem::Decorations::NewLine;
}

bool MarkdownParser::ParseSeparator(const char* line, int start, int end)
{
  if (mItems.empty()) return false;

  char separator = line[start];
  bool plainLine = true;
  for(++end; --end > start; )
    if (line[end] != separator)
    {
      plainLine = false;
      break;
    }

  if (plainLine)
  {
    MarkdownItem& item = Previous();
    // Empty line or Headered line => separator
    if (item.HasEmptyLine() || item.Header() != 0)
    {
      item.mDecoration |= MarkdownItem::Decorations::HorizontalRule;
    }
    else // Last line has to be be headered
    {
      MarkdownItem::FontSize fontSize = (separator == '=') ? MarkdownItem::FontSize::H1 : MarkdownItem::FontSize::H2;
      int lineEnd = LastLineEnd((int)mItems.size() - 1);
      int lineStart = LastLineStart(lineEnd);
      for(++lineEnd; --lineEnd >= lineStart; )
      {
        mItems[lineEnd].mFontSize = fontSize;
        mItems[lineEnd].mFontType |= MarkdownItem::FontType::Bold;
      }
    }
  }

  return plainLine;
}

MarkdownItem::FontSize MarkdownParser::ParseHeader(const char* line, int& start, int& end)
{
  int startText = start;
  int endText = end;

  // Seek end of #
  while((startText < endText) && (line[startText] == '#')) ++startText;
  if (startText >= endText) return MarkdownItem::FontSize::Normal;
  MarkdownItem::FontSize fontSize = (MarkdownItem::FontSize)(startText - start);
  if (fontSize > MarkdownItem::FontSize::H4) fontSize = MarkdownItem::FontSize::H4;

  // Check space/tab
  if (line[startText] != ' ' && line[startText] != '\t') return MarkdownItem::FontSize::Normal;
  while(line[startText] == ' ' || line[startText] == '\t') ++startText; // Skip all space

  // Seek last #
  while((startText < endText) && (line[endText] == '#')) --endText;
  if (startText < endText)
    // Check Delete all tailing space/tab & validate - start & stop at the same char (empty header) accepted
    if (line[endText] == ' ' || line[endText] == '\t')
    {
      while (line[endText] == ' ' || line[endText] == '\t') --endText;
      if (endText < startText) endText = startText; // Make end not lesser than start
      end = endText; // Assign new end position
    }

  // Validate start
  start = startText;
  return fontSize;
}

MarkdownItem::Decorations MarkdownParser::ParseBullet(const char* line, int& start, int end)
{
  int startText = start;
  if (end >= startText + 2)
    if (line[startText] == '-' || line[startText] == '+' || line[startText] == '*')
      if (line[startText + 1] == ' ' || line[startText + 1] == '\t')
      {
        startText += 2;
        while ((startText < end) && (line[startText] == ' ' || line[startText] == '\t')) ++startText; // Skip all spaces
        start = startText;
        return MarkdownItem::Decorations::Bullet;
      }
  return MarkdownItem::Decorations::None;
}

MarkdownItem::Decorations MarkdownParser::ParseNumberedBullets(const char* line, int& start, int end, int& number)
{
  int integer = 0;
  int startText = start;
  if (end >= startText + 2)
    if (line[startText] >= '0' && line[startText] <= '9')
    {
      while(line[startText] >= '0' && line[startText] <= '9') integer = integer * 10 + (line[startText++] - 0x30);
      if ((line[startText] == '.') && (line[startText + 1] == ' ' || line[startText + 1] == '\t'))
      {
        startText += 2;
        while ((startText < end) && (line[startText] == ' ' || line[startText] == '\t')) ++startText; // Skip all spaces
        number = integer;
        start = startText;
        return MarkdownItem::Decorations::Numbered;
      }
    }
  return MarkdownItem::Decorations::None;
}

MarkdownItem& MarkdownParser::New()
{
  mItems.push_back(sNullItem);
  return mItems.back();
}

MarkdownItem& MarkdownParser::Previous()
{
  if (!mItems.empty()) return mItems.back();
  static MarkdownItem sTrashItem(0, 0,0,
                                 MarkdownItem::Decorations::None,
                                 MarkdownItem::FontSize::Normal,
                                 MarkdownItem::FontType::None);
  return sTrashItem;
}

int MarkdownParser::LastLineEnd(int from)
{
  if ((unsigned int)from < mItems.size())
    for(++from; --from >= 0; )
      if (mItems[from].HasNewLine())
        return from;
  return -1;
}

int MarkdownParser::LastLineStart(int from)
{
  if ((unsigned int)from < mItems.size())
    for(++from; --from >= 0; )
      if ((from == 0) || mItems[from - 1].HasNewLine())
        return from;
  return -1;
}

void MarkdownParser::EmptyLine()
{
  Previous().mDecoration |= MarkdownItem::Decorations::EmptyLine;
}

void MarkdownParser::ParseText(const char* line, int start, int end, MarkdownItem::Decorations decoration, MarkdownItem::FontSize fontSize, int number)
{
  // Keep indexes of open styles so that it's easier to remove unclosed styles. -1 means no open style yet
  Indexes indexes;

  mTokens.Clear();
  mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=(unsigned short)end, .Type=MarkdownItem::FontType::None, .Marker=0, .MarkerSize=0 });

  for(; start <= end; ++start)
  {
    char c1 = line[start];
    // Backslash ?
    if (c1 == '\\') { ++start; continue; }
    char c2 = (start < end) ? (char)line[start + 1] : (char)0;
    char c3 = (start < end - 1) ? (char)line[start + 2] : (char)0;

    // Fixed font
    if (c1 == '`') { start = ParseAntiQuote(line, start, end); continue; }

    // Underscore italic + underline
    if (c1 == c2 && c1 == c3 && c1 == '_') { RecordItalicUnderlineTriUnderscore(indexes, start); start += 2; }
    // Simple underscore
    else if (c1 == c2 && c1 == '_') { RecordUnderlineUnderscore(indexes, start); ++start; }
    // Simple italic
    else if (c1 == '_') RecordItalicUnderscore(indexes, start);
    // Strike thru
    else if (c1 == c2 && c1 == '~') { RecordStrikeThruTilda(indexes, start); ++start; }
    // Star italic + bold
    else if (c1 == c2 && c1 == c3 && c1 == '*') { RecordBoldItalicTristar(indexes, start); start += 2; }
    // Double star Bold
    else if (c1 == c2 && c1 == '*') { RecordBoldStar(indexes, start); ++start; }
    // Single star Italic
    else if (c1 == '*') RecordItalicStar(indexes, start);
  }

  // Remove unclosed tokens
  for(int i = mTokens.Count(); --i >= 0;)
    if (mTokens[i].StopOffset == 0)
      mTokens.Delete(i);

  MarkdownItem::FontType forcedType = (fontSize == MarkdownItem::FontSize::Normal) ?
                                      MarkdownItem::FontType::None :
                                      MarkdownItem::FontType::Bold;
  int firstItem = (int)mItems.size();
  if (TokensToItems(0, line, decoration, fontSize, MarkdownItem::FontType::None, number) != mTokens.Count())
    LOG(LogError) << "[Markdown] Parsing error!";
  for(int i = (int)mItems.size(); --i >= firstItem; )
    mItems[i].mFontType |= forcedType;
}

int MarkdownParser::ParseAntiQuote(const char* line, int start, int end)
{
  for(int next = start; ++next <= end; )
    if (line[next] == '`')
      {
        mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=(unsigned short)next, .Type=MarkdownItem::FontType::Fixed, .Marker='`', .MarkerSize=1 });
        return next;
      }

  return start;
}

void MarkdownParser::SplitItems(const char* line, int start, int stop, int number, MarkdownItem::Decorations decorations,
                                MarkdownItem::FontSize size, MarkdownItem::FontType type)
{
  // Fixed/code spans are literal: backslashes must not be treated as escape characters.
  // Without this guard, a single backslash as content (e.g. `\`) causes start > stop
  // after the "Kill first escaping" step below, passing a negative size to memchr.
  if (((int)type & (int)MarkdownItem::FontType::Fixed) != 0)
  {
    mItems.push_back(MarkdownItem(start, stop, number, decorations, size, type));
    return;
  }

  // Kill first escaping
  if (line[start] == '\\') ++start;
  for(;;)
  {
    const char* escape = (const char*)memchr(&line[start], '\\', stop - start); // Exclude last char
    if (escape != nullptr)
    {
      int newEnd = start + (int)(escape - &line[start]) - 1;
      mItems.push_back(MarkdownItem(start, newEnd, number, decorations, size, type));
      start = newEnd + 2;
      continue;
    }
    mItems.push_back(MarkdownItem(start, stop, number, decorations, size, type));
    break;
  }
}

int MarkdownParser::TokensToItems(int index, const char* line, MarkdownItem::Decorations decorations, MarkdownItem::FontSize size, MarkdownItem::FontType type, int number)
{
  int currentIndex = index;
  const MarkdownParser::Token& current = mTokens(currentIndex++);
  int start = current.StartOffset + current.MarkerSize;
  int stop = current.StopOffset - current.MarkerSize;

  while(currentIndex < mTokens.Count())
  {
    const MarkdownParser::Token& next = mTokens(currentIndex);
    if (next.StartOffset > stop) break;

    // Add the text between the current position and the start of next chunk
    if (next.StartOffset > start)
    {
      SplitItems(line, start, next.StartOffset - 1, number, decorations, size, type | current.Type);
      decorations &= ~(MarkdownItem::Decorations::Bullet | MarkdownItem::Decorations::Numbered);
    }

    // Recursive call
    int oldIndex = currentIndex;
    currentIndex += TokensToItems(currentIndex, line, decorations, size, type | current.Type, number);
    decorations &= ~(MarkdownItem::Decorations::Bullet | MarkdownItem::Decorations::Numbered);

    // Reset start
    start = mTokens[oldIndex].StopOffset + 1;
  }

  // Remaining chars?
  if (start <= stop)
  {
    SplitItems(line, start, stop, number, decorations, size, type | current.Type);
    decorations &= ~(MarkdownItem::Decorations::Bullet | MarkdownItem::Decorations::Numbered);
  }

  // Return length
  return currentIndex - index;
}

void MarkdownParser::RecordItalicUnderscore(Indexes& indexes, int start)
{
  if (indexes.LastOpenItalicUnder < 0)
  {
    indexes.LastOpenItalicUnder = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Italic, .Marker='_', .MarkerSize=1 });
  }
  else
  {
    mTokens(indexes.LastOpenItalicUnder).StopOffset = (unsigned short)start;
    indexes.CloseItalicUnder();
  }
}

void MarkdownParser::RecordStrikeThruTilda(MarkdownParser::Indexes& indexes, int start)
{
  if (indexes.LastOpenStrike < 0)
  {
    indexes.LastOpenStrike = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::StrikeThru, .Marker='~', .MarkerSize=2 });
  }
  else
  {
    mTokens(indexes.LastOpenStrike).StopOffset = (unsigned short)(start + 1);
    indexes.CloseStrikeThru();
  }
}

void MarkdownParser::RecordBoldItalicTristar(MarkdownParser::Indexes& indexes, int start)
{
  // Close a one star Italic
  if (indexes.LastOpenBold >= 0 && indexes.LastOpenItalicStar < 0)
  {
    mTokens(indexes.LastOpenBold).StopOffset = (unsigned short)start;
    indexes.CloseBold();
  }
  // Close a double star bold
  else if (indexes.LastOpenBold < 0 && indexes.LastOpenItalicStar >= 0)
  {
    mTokens(indexes.LastOpenItalicStar).StopOffset = (unsigned short)(start + 1);
    indexes.CloseItalicStar();
  }
  // Close both italic & bold
  else if (indexes.LastOpenBold >= 0 && indexes.LastOpenItalicStar >= 0)
  {
    // Tristar also
    if (indexes.LastOpenBold == indexes.LastOpenItalicStar)
    {
      mTokens(indexes.LastOpenBold).StopOffset = (unsigned short)(start + 2);
      indexes.CloseBold();
      indexes.CloseItalicStar();
    }
    // Bold opened last
    else if (indexes.LastOpenBold > indexes.LastOpenItalicStar)
    {
      mTokens(indexes.LastOpenBold).StopOffset = (unsigned short)(start + 1);
      mTokens(indexes.LastOpenItalicStar).StopOffset = (unsigned short)(start + 2);
      indexes.CloseBold();
      indexes.CloseItalicStar();
    }
    // Italic opened last
    else
    {
      mTokens(indexes.LastOpenItalicStar).StopOffset = (unsigned short)start;
      mTokens(indexes.LastOpenBold).StopOffset = (unsigned short)(start + 2);
      indexes.CloseItalicStar();
      indexes.CloseBold();
    }
  }
  // Open italic & bold
  else
  {
    indexes.LastOpenItalicStar = indexes.LastOpenBold = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Italic | MarkdownItem::FontType::Bold, .Marker='*', .MarkerSize=3 });
  }
}

void MarkdownParser::RecordBoldStar(MarkdownParser::Indexes& indexes, int start)
{
  // Open new bold
  if (indexes.LastOpenBold < 0)
  {
    indexes.LastOpenBold = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Bold, .Marker='*', .MarkerSize=2 });
  }
  else // Close bold
  {
    // Last bold is a bold-only index?
    if (mTokens[indexes.LastOpenBold].Type == MarkdownItem::FontType::Bold)
    {
      mTokens(indexes.LastOpenBold).StopOffset = (unsigned short)(start + 1);
      indexes.CloseBold();
    }
    else // Or a multi-type
    {
      mTokens.Insert(mTokens[indexes.LastOpenBold], indexes.LastOpenBold);
      MarkdownParser::Token& italic = mTokens(indexes.LastOpenBold);
      MarkdownParser::Token& bold = mTokens(indexes.LastOpenBold + 1);
      // Made the first an italic only marker
      italic.Type = MarkdownItem::FontType::Italic;
      italic.MarkerSize = 1;
      // Made the second a bold only and close it
      bold.StartOffset++;
      bold.StopOffset = start + 1;
      bold.Type = MarkdownItem::FontType::Bold;
      indexes.CloseBold();
    }
  }
}

void MarkdownParser::RecordItalicStar(MarkdownParser::Indexes& indexes, int start)
{
  // Open new italic
  if (indexes.LastOpenItalicStar < 0)
  {
    indexes.LastOpenItalicStar = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Italic, .Marker='*', .MarkerSize=1 });
  }
  else // Close italic
  {
    // Last italic is a italic-only index?
    if (mTokens[indexes.LastOpenItalicStar].Type == MarkdownItem::FontType::Italic)
    {
      mTokens(indexes.LastOpenItalicStar).StopOffset = (unsigned short)start;
      indexes.CloseItalicStar();
    }
    else // Or a multi-type
    {
      mTokens.Insert(mTokens[indexes.LastOpenItalicStar], indexes.LastOpenItalicStar);
      MarkdownParser::Token& italic = mTokens(indexes.LastOpenItalicStar);
      MarkdownParser::Token& bold = mTokens(indexes.LastOpenItalicStar + 1);
      // Made the first a bold only marker
      italic.Type = MarkdownItem::FontType::Bold;
      italic.MarkerSize = 2;
      // Made the second an italic only and close it
      bold.StartOffset += 2;
      bold.StopOffset = start;
      bold.Type = MarkdownItem::FontType::Italic;
      indexes.CloseItalicStar();
    }
  }
}

void MarkdownParser::RecordItalicUnderlineTriUnderscore(MarkdownParser::Indexes& indexes, int start)
{
  // Close a one underscore Italic
  if (indexes.LastOpenItalicUnder >= 0 && indexes.LastOpenUnderline < 0)
  {
    mTokens(indexes.LastOpenItalicUnder).StopOffset = (unsigned short)start;
    indexes.CloseItalicUnder();
  }
  // Close a double underscore underline
  else if (indexes.LastOpenUnderline < 0 && indexes.LastOpenItalicUnder >= 0)
  {
    mTokens(indexes.LastOpenUnderline).StopOffset = (unsigned short)(start + 1);
    indexes.CloseUnderline();
  }
  // Close both italic & bold
  else if (indexes.LastOpenUnderline >= 0 && indexes.LastOpenItalicUnder >= 0)
  {
    // Triunderscore also
    if (indexes.LastOpenUnderline == indexes.LastOpenItalicUnder)
    {
      mTokens(indexes.LastOpenUnderline).StopOffset = (unsigned short)(start + 2);
      indexes.CloseUnderline();
      indexes.CloseItalicUnder();
    }
    // Underline opened last
    else if (indexes.LastOpenUnderline > indexes.LastOpenItalicUnder)
    {
      mTokens(indexes.LastOpenUnderline).StopOffset = (unsigned short)(start + 1);
      mTokens(indexes.LastOpenItalicUnder).StopOffset = (unsigned short)(start + 2);
      indexes.CloseUnderline();
      indexes.CloseItalicUnder();
    }
    // Italic opened last
    else
    {
      mTokens(indexes.LastOpenItalicUnder).StopOffset = (unsigned short)start;
      mTokens(indexes.LastOpenUnderline).StopOffset = (unsigned short)(start + 2);
      indexes.CloseItalicUnder();
      indexes.CloseUnderline();
    }
  }
  // Open italic & bold
  else
  {
    indexes.LastOpenItalicUnder = indexes.LastOpenUnderline = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Italic | MarkdownItem::FontType::Underline, .Marker='_', .MarkerSize=3 });
  }
}

void MarkdownParser::RecordUnderlineUnderscore(MarkdownParser::Indexes& indexes, int start)
{
  // Open new underline
  if (indexes.LastOpenUnderline < 0)
  {
    indexes.LastOpenUnderline = mTokens.Count();
    mTokens.Add({ .StartOffset=(unsigned short)start, .StopOffset=0, .Type=MarkdownItem::FontType::Underline, .Marker='_', .MarkerSize=2 });
  }
  else // Close underline
  {
    // Last bold is a italic-only index?
    if (mTokens[indexes.LastOpenUnderline].Type == MarkdownItem::FontType::Underline)
    {
      mTokens(indexes.LastOpenUnderline).StopOffset = (unsigned short)(start + 1);
      indexes.CloseUnderline();
    }
    else // Or a multi-type
    {
      mTokens.Insert(mTokens[indexes.LastOpenUnderline], indexes.LastOpenUnderline);
      MarkdownParser::Token& italic = mTokens(indexes.LastOpenUnderline);
      MarkdownParser::Token& underline = mTokens(indexes.LastOpenUnderline + 1);
      // Made the first an italic only marker
      italic.Type = MarkdownItem::FontType::Italic;
      italic.MarkerSize = 1;
      // Made the second a bold only and close it
      underline.StartOffset++;
      underline.StopOffset = start + 1;
      underline.Type = MarkdownItem::FontType::Underline;
      indexes.CloseUnderline();
    }
  }
}


