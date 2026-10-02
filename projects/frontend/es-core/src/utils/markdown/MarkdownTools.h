//
// Created by bkg2k on 04/11/25.
//
#pragma once

#include <utils/markdown/MarkdownItem.h>
#include <utils/String.h>

class MarkdownTools
{
  public:
    //! Default constructor
    MarkdownTools() : mEnabled(true) {}

    /*!
     * @brief Enable or disable styles
     * @param enabled New enable state
     * @return Previous enable state
     */
    bool SetEnabled(bool enabled) { bool current = mEnabled; mEnabled = enabled; return current; }

    /*!
     * @brief Apply markdown styles around the given source string
     * @param source Source to stylize
     * @param style Styles to apply
     * @return Stylized string
     */
    [[nodiscard]] String Stylize(const String& source, MarkdownItem::FontType style) const
    {
      if (!mEnabled) return source;
      String result;
      if (hasFlag(style, MarkdownItem::FontType::Bold)) result.Append("**", 2);
      if (hasFlag(style, MarkdownItem::FontType::Italic)) result.Append('*');
      if (hasFlag(style, MarkdownItem::FontType::StrikeThru)) result.Append("~~", 2);
      if (hasFlag(style, MarkdownItem::FontType::Underline)) result.Append("__", 2);
      if (hasFlag(style, MarkdownItem::FontType::Fixed)) result.Append('`');
      result.Append(source);
      if (hasFlag(style, MarkdownItem::FontType::Fixed)) result.Append('`');
      if (hasFlag(style, MarkdownItem::FontType::Underline)) result.Append("__", 2);
      if (hasFlag(style, MarkdownItem::FontType::StrikeThru)) result.Append("~~", 2);
      if (hasFlag(style, MarkdownItem::FontType::Italic)) result.Append('*');
      if (hasFlag(style, MarkdownItem::FontType::Bold)) result.Append("**", 2);
      return result;
    }

    /*!
     * @brief Apply markdown bold style around the given string
     * @param source Source to stylize
     * @return Stylized string
     */
    [[nodiscard]] String Bold(const String& source) const { if (!mEnabled) return source; return String("**", 2).Append(source).Append("**", 2); }
    /*!
     * @brief Apply markdown italic style around the given string
     * @param source Source to stylize
     * @return Stylized string
     */
    [[nodiscard]] String Italic(const String& source) const { if (!mEnabled) return source; return String('*').Append(source).Append('*'); }
    /*!
     * @brief Apply markdown strike thru style around the given string
     * @param source Source to stylize
     * @return Stylized string
     */
    [[nodiscard]] String StrikeThru(const String& source) const { if (!mEnabled) return source; return String("~~", 2).Append(source).Append("~~", 2); }
    /*!
     * @brief Apply markdown underline style around the given string (non standard)
     * @param source Source to stylize
     * @return Stylized string
     */
    [[nodiscard]] String Underline(const String& source) const { if (!mEnabled) return source; return String("__", 2).Append(source).Append("__", 2); }
    /*!
     * @brief Apply markdown fixed style around the given string
     * @param source Source to stylize
     * @return Stylized string
     */
    [[nodiscard]] String Fixed(const String& source) const { if (!mEnabled) return source; return String('`').Append(source).Append('`'); }

  private:
    //! Styles enabled ?
    bool mEnabled;
};