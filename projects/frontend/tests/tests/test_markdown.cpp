#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-function-cognitive-complexity"
//
// Created by bkg2k on 21/04/2026.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <utils/markdown/MarkdownParser.h>

class MarkdownParserTest : public ::testing::Test
{
  protected:
    static std::string ItemText(const String& src, const MarkdownItem& item)
    {
      if (item.TextStartIndex() > item.TextEndIndex()) return "";
      return std::string(src.data() + item.TextStartIndex(),
                         (size_t)(item.TextEndIndex() - item.TextStartIndex() + 1));
    }
};

// ─── Plain text ──────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, PlainText)
{
  String s("hello world");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "hello world");
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_FALSE(p.Item(0).IsItalic());
  ASSERT_FALSE(p.Item(0).IsFixed());
  ASSERT_FALSE(p.Item(0).IsStrikeTrhu());
  ASSERT_FALSE(p.Item(0).IsUnderline());
  ASSERT_FALSE(p.Item(0).HasBullet());
  ASSERT_FALSE(p.Item(0).HasNumbered());
  ASSERT_EQ(p.Item(0).Header(), 0);
  //ASSERT_TRUE(p.Item(0).HasNewLine());
}

// ─── Bold ─────────────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, Bold)
{
  String s("**bold**");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "bold");
  ASSERT_TRUE(p.Item(0).IsBold());
  ASSERT_FALSE(p.Item(0).IsItalic());
}

TEST_F(MarkdownParserTest, BoldUnclosed)
{
  // Unclosed markers produce plain text (token removed)
  String s("**unclosed");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_EQ(ItemText(s, p.Item(0)), "**unclosed");
}

// ─── Italic ───────────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, ItalicStar)
{
  String s("*italic*");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "italic");
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(0).IsItalic());
}

TEST_F(MarkdownParserTest, ItalicUnderscore)
{
  String s("_italic_");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "italic");
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(0).IsItalic());
}

// ─── Bold + Italic ────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, BoldItalicTristar)
{
  String s("***text***");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "text");
  ASSERT_TRUE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(0).IsItalic());
}

// ─── Strike-through ───────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, StrikeThrough)
{
  String s("~~strike~~");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "strike");
  ASSERT_TRUE(p.Item(0).IsStrikeTrhu());
  ASSERT_FALSE(p.Item(0).IsBold());
}

// ─── Underline ────────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, Underline)
{
  String s("__underline__");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "underline");
  ASSERT_TRUE(p.Item(0).IsUnderline());
  ASSERT_FALSE(p.Item(0).IsBold());
}

// ─── Fixed font (backtick) ────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, FixedFont)
{
  String s("`code`");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "code");
  ASSERT_TRUE(p.Item(0).IsFixed());
  ASSERT_FALSE(p.Item(0).IsBold());
}

TEST_F(MarkdownParserTest, FixedFontInLine)
{
  // "text `code` more" — backtick span in the middle
  String s("text `code` more");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 3);
  ASSERT_EQ(ItemText(s, p.Item(0)), "text ");
  ASSERT_FALSE(p.Item(0).IsFixed());
  ASSERT_EQ(ItemText(s, p.Item(1)), "code");
  ASSERT_TRUE(p.Item(1).IsFixed());
  ASSERT_EQ(ItemText(s, p.Item(2)), " more");
  ASSERT_FALSE(p.Item(2).IsFixed());
}

TEST_F(MarkdownParserTest, FixedFontBackslashIsLiteral)
{
  // Inside backticks, backslash must NOT be treated as escape — it is literal.
  String s("`\\test`");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).IsFixed());
  // Content starts at position 1 (after opening backtick) and ends at position 5.
  // The backslash at position 1 is included verbatim.
  ASSERT_EQ(ItemText(s, p.Item(0)), "\\test");
}

TEST_F(MarkdownParserTest, FixedFontSingleBackslash_NoCrash)
{
  // Regression: `\` (backtick, single backslash, backtick) used to crash via
  // memchr receiving a negative size after "kill first escaping" ran past stop.
  String s("`\\`");
  MarkdownParser p; p.Parse(s);  // must not crash
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).IsFixed());
  ASSERT_EQ(ItemText(s, p.Item(0)), "\\");
}

TEST_F(MarkdownParserTest, FixedFontUnclosed)
{
  // No closing backtick: backtick is treated as plain text (no token added)
  String s("`noquote");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_FALSE(p.Item(0).IsFixed());
}

// ─── Headers ──────────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, Header1)
{
  String s("# Title");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "Title");
  ASSERT_EQ(p.Item(0).Header(), 1);
  ASSERT_TRUE(p.Item(0).IsBold());  // headers are forced bold
}

TEST_F(MarkdownParserTest, Header2)
{
  String s("## Title");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(p.Item(0).Header(), 2);
  ASSERT_TRUE(p.Item(0).IsBold());
}

TEST_F(MarkdownParserTest, Header3)
{
  String s("### Title");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(p.Item(0).Header(), 3);
}

TEST_F(MarkdownParserTest, Header4)
{
  String s("#### Title");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(p.Item(0).Header(), 4);
}

TEST_F(MarkdownParserTest, H2ViaUnderline)
{
  // A line of dashes below plain text promotes that text to H2+Bold
  String s("text\n---");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "text");
  ASSERT_EQ(p.Item(0).Header(), 2);
  ASSERT_TRUE(p.Item(0).IsBold());
}

TEST_F(MarkdownParserTest, H1ViaUnderline)
{
  // A line of equals signs below plain text promotes that text to H1+Bold
  String s("text\n===");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(p.Item(0).Header(), 1);
  ASSERT_TRUE(p.Item(0).IsBold());
}

// ─── Bullet lists ─────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, BulletDash)
{
  String s("- item");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "item");
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_FALSE(p.Item(0).HasNumbered());
}

TEST_F(MarkdownParserTest, BulletPlus)
{
  String s("+ item");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).HasBullet());
}

TEST_F(MarkdownParserTest, BulletStar)
{
  // "* item" — star followed by space is a bullet, not italic
  String s("* item");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_FALSE(p.Item(0).IsItalic());
}

TEST_F(MarkdownParserTest, BulletMultipleLines)
{
  String s("- one\n- two");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_TRUE(p.Item(1).HasBullet());
  ASSERT_EQ(ItemText(s, p.Item(0)), "one");
  ASSERT_EQ(ItemText(s, p.Item(1)), "two");
}

// ─── Numbered lists ───────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, NumberedList)
{
  String s("1. item");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_EQ(ItemText(s, p.Item(0)), "item");
  ASSERT_TRUE(p.Item(0).HasNumbered());
  ASSERT_FALSE(p.Item(0).HasBullet());
  ASSERT_EQ(p.Item(0).Number(), 1);
}

TEST_F(MarkdownParserTest, NumberedListCustomNumber)
{
  String s("5. other");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).HasNumbered());
  ASSERT_EQ(p.Item(0).Number(), 5);
}

TEST_F(MarkdownParserTest, NumberedListMultiple)
{
  String s("1. first\n2. second");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  ASSERT_EQ(p.Item(0).Number(), 1);
  ASSERT_EQ(p.Item(1).Number(), 2);
  ASSERT_TRUE(p.Item(0).HasNumbered());
  ASSERT_TRUE(p.Item(1).HasNumbered());
}

// ─── Decorations: NewLine / EmptyLine / HorizontalRule ───────────────────────

TEST_F(MarkdownParserTest, NewLineDecoration)
{
  String s("line1\nline2");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  ASSERT_TRUE(p.Item(0).HasNewLine());
  //ASSERT_TRUE(p.Item(1).HasNewLine());
}

TEST_F(MarkdownParserTest, EmptyLineDecoration)
{
  String s("line1\n\nline2");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  ASSERT_TRUE(p.Item(0).HasEmptyLine());   // decoration on the item before the blank line
  ASSERT_FALSE(p.Item(1).HasEmptyLine());
}

TEST_F(MarkdownParserTest, HorizontalRule)
{
  // A separator line after an empty line adds HorizontalRule to the preceding item
  String s("ab\n\n---");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).HasHorizontalRule());
}

// ─── Mixed content ────────────────────────────────────────────────────────────

TEST_F(MarkdownParserTest, TextThenBoldThenText)
{
  String s("text **bold** more");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 3);
  ASSERT_EQ(ItemText(s, p.Item(0)), "text ");
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_EQ(ItemText(s, p.Item(1)), "bold");
  ASSERT_TRUE(p.Item(1).IsBold());
  ASSERT_EQ(ItemText(s, p.Item(2)), " more");
  ASSERT_FALSE(p.Item(2).IsBold());
}

TEST_F(MarkdownParserTest, BulletWithFixedFont)
{
  // "- `code`" — bullet + fixed font, only first item should carry the bullet
  String s("- `code`");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_TRUE(p.Item(0).IsFixed());
  ASSERT_EQ(ItemText(s, p.Item(0)), "code");
}

TEST_F(MarkdownParserTest, BulletBoldThenPlain_BulletNotDoubled)
{
  // Regression: "- **Title:**Song" used to produce TWO items both with the
  // Bullet decoration because decorations was not cleared after the recursive
  // TokensToItems call that consumed the bold token at the start of the line.
  String s("- **Title:**Song");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  // First item: the bold part — carries the bullet
  ASSERT_EQ(ItemText(s, p.Item(0)), "Title:");
  ASSERT_TRUE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(0).HasBullet());
  // Second item: plain text after the bold — must NOT carry the bullet
  ASSERT_EQ(ItemText(s, p.Item(1)), "Song");
  ASSERT_FALSE(p.Item(1).IsBold());
  ASSERT_FALSE(p.Item(1).HasBullet());
}

TEST_F(MarkdownParserTest, BulletBoldOnlyItem_SingleBullet)
{
  // Entire bullet content is bold: "- **all bold**"
  String s("- **all bold**");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_EQ(ItemText(s, p.Item(0)), "all bold");
}

TEST_F(MarkdownParserTest, BulletTextThenBold_OnlyFirstHasBullet)
{
  // "- plain **bold**" — plain text comes first, then bold
  String s("- plain **bold**");
  MarkdownParser p; p.Parse(s);
  ASSERT_EQ(p.ItemCount(), 2);
  ASSERT_TRUE(p.Item(0).HasBullet());
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_FALSE(p.Item(1).HasBullet());
  ASSERT_TRUE(p.Item(1).IsBold());
}

TEST_F(MarkdownParserTest, NestedBoldInItalic)
{
  // "_italic **bold+italic** more_" — bold nested inside italic
  String s("_italic **bold** more_");
  MarkdownParser p; p.Parse(s);
  // Expect three items: italic-only, bold+italic, italic-only
  ASSERT_EQ(p.ItemCount(), 3);
  ASSERT_TRUE(p.Item(0).IsItalic());
  ASSERT_FALSE(p.Item(0).IsBold());
  ASSERT_TRUE(p.Item(1).IsItalic());
  ASSERT_TRUE(p.Item(1).IsBold());
  ASSERT_TRUE(p.Item(2).IsItalic());
  ASSERT_FALSE(p.Item(2).IsBold());
}

TEST_F(MarkdownParserTest, BoldInsideFixedFont_FixedWins)
{
  // Inside backticks, ** markers must not be processed — content is literal
  String s("`no **bold** here`");
  MarkdownParser p; p.Parse(s);
  // ParseAntiQuote jumps over everything inside, so no bold token is created
  ASSERT_EQ(p.ItemCount(), 1);
  ASSERT_TRUE(p.Item(0).IsFixed());
  ASSERT_FALSE(p.Item(0).IsBold());
}

#pragma clang diagnostic pop
