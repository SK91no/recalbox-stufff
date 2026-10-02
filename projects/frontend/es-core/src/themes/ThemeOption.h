//
// Created by bkg2k on 26/06/25.
//
#pragma once

#include <algorithm>
#include "utils/locale/LocaleHelper.h"
#include <utils/String.h>
#include <utils/storage/HashMap.h>
#include <utils/storage/Set.h>
#include <utils/os/system/Mutex.h>

struct ThemeOption
{
  public:
    struct Value
    {
      private:
        String mRaw;        //!< Raw value (english or non-translated)
        String mTranslated; //!< Translated value
        int mIndex;         //!< extracted index
        int mTextStart;     //!< Position where text starts (after index prefix)
        bool mIndexed;      //!< Index is valid ?

        //! Extract index
        void ExtractIndex()
        {
          bool hasSeparator = (unsigned int)mRaw.Find('.') < 8 || (unsigned int)mRaw.Find('-') < 8;
          if (!hasSeparator) return;
          size_t pos = mRaw.find_first_not_of("0123456789");
          if (pos == 0) return;
          char end = (pos == std::string::npos) ? '\0' : mRaw[pos];
          if (end != ' ' && end != '.' && end != '-') return;
          mIndex = mRaw.AsInt(end);
          mIndexed = true;
          // Calculate text start position (skip spaces, separator, and more spaces)
          mTextStart = (int)pos;
          while (mTextStart < (int)mRaw.size() && mRaw[mTextStart] == ' ') mTextStart++;
          if (mTextStart < (int)mRaw.size() && (mRaw[mTextStart] == '-' || mRaw[mTextStart] == '.')) mTextStart++;
          while (mTextStart < (int)mRaw.size() && mRaw[mTextStart] == ' ') mTextStart++;
        }

      public:
        //! Constructor
        Value(const String& raw, const String& translated) : mRaw(raw), mTranslated(translated), mIndex(0), mTextStart(0), mIndexed(false) { ExtractIndex(); }
        //! Get raw string
        [[nodiscard]] const String& Raw() const { return mRaw; }
        //! Get translated, or raw if no translation has been recorded
        [[nodiscard]] const String& Translated() const { return mTranslated.empty() ? mRaw : mTranslated; }
        [[nodiscard]] int Index() const { return mIndex; }
        [[nodiscard]] int TextStart() const { return mTextStart; }
        [[nodiscard]] bool IsIndexed() const { return mIndexed; }
    };

    /*!
     * @brief Constructor
     * @param name Subset raw name
     */
    explicit ThemeOption(const String& name)  : mRawName(name) { Mutex::AutoLock locker(sGlobalGuardian); mIndex = ++sGlobalIndex; }

    /*!
     * @brief Set user-friendly name
     * @param displayableName Name used in menus
     */
    ThemeOption& SetUserFriendlyName(const String& displayableName) { mUserFriendlyName = displayableName; return *this; }

    /*!
     * @brief Set user-friendly help message
     * @param help Help use din menus
     */
    ThemeOption& SetUserFriendlyHelp(const String& help) { mHelp = help; return *this; }

    /*!
     * @brief Add a value to the option value list
     * @param value Value to add
     */
    ThemeOption& AddValue(const String& raw, const String& translated) { mValues[raw] = translated; return *this; }

    //! Available?
    [[nodiscard]] bool IsValid() const { return !mValues.empty(); }

    //! Finalize & validate
    ThemeOption& Finalize()
    {
      // Special option for gameclips

      if (!mValues.empty() && mRawName == "gameclipview") mValues.insert("0 - DEFAULT", "0 - DEFAULT");

      // Auto-translate old-default theme options
      if (mRawName == "colorset"    ) { SetUserFriendlyName(_("THEME'S COLORSET"));            SetUserFriendlyHelp(_("Select a colorset from this theme.")); }
      if (mRawName == "iconset"     ) { SetUserFriendlyName(_("THEME'S ICONSET"));             SetUserFriendlyHelp(_("Select an iconset from this theme.")); }
      if (mRawName == "menu"        ) { SetUserFriendlyName(_("THEME'S MENU STYLE"));          SetUserFriendlyHelp(_("Select menu style from this theme.")); }
      if (mRawName == "systemview"  ) { SetUserFriendlyName(_("THEME'S SYSTEMVIEW LAYOUT"));   SetUserFriendlyHelp(_("Select system view layout from this theme.")); }
      if (mRawName == "gamelistview") { SetUserFriendlyName(_("THEME'S GAMELISTVIEW LAYOUT")); SetUserFriendlyHelp(_("Select gamelist view layout from this theme.")); }
      if (mRawName == "gameclipview") { SetUserFriendlyName(_("THEME'S GAMECLIPVIEW LAYOUT")); SetUserFriendlyHelp(_("Select gameclip view layout from this theme.")); }

      return *this;
    }

    //! Index
    [[nodiscard]] int Index() const { return mIndex; }

    //! Get name
    [[nodiscard]] const String& Name() const { return mRawName; }
    //! Get translated name
    [[nodiscard]] const String& DisplayName() const { return mUserFriendlyName.empty() ? mRawName : mUserFriendlyName; }
    //! Get translated help
    [[nodiscard]] const String& Help() const { return mHelp; }
    //! Get value list
    [[nodiscard]] const HashMap<String, String>& Values() const { return mValues; }
    //! Get value list
    [[nodiscard]] std::vector<Value> SortedValue() const
    {
      std::vector<Value> result;

      // Get raw list
      for(const auto& value : Values()) result.push_back({ value.first, value.second});
      // Try numeric sorting, else  use an alphanumeric sort
      if (!TrySortNumerically(result)) std::ranges::sort(result, [] (const Value& a, const Value& b) -> bool { return a.Raw() < b.Raw(); });

      return result;
    }

  private:
    //! Increasing index
    static int sGlobalIndex;
    //! Global index protector
    static Mutex sGlobalGuardian;

    //! Subset name
    String mRawName;
    //! Translated name for menu & other display
    String mUserFriendlyName;
    //! Help string
    String mHelp;
    //! Value list ( raw => translated )
    HashMap<String, String> mValues;
    //! Index
    int mIndex;

    static bool TrySortNumerically(std::vector<Value>& list)
    {
      bool allNumeric = true;
      for(const Value& item : list) allNumeric &= item.IsIndexed();

      if (allNumeric)
        std::ranges::sort(list, [] (const Value& a, const Value& b) -> bool { return a.Index() < b.Index(); });
      return allNumeric;
    }
};