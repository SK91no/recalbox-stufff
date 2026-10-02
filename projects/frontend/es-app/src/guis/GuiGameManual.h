//
// Created by bkg2k on 06/02/2026.
//
#pragma once
#include "components/ComponentGrid.h"
#include "components/Grid.h"
#include "components/MarkdownText.h"
#include "components/NinePatchComponent.h"
#include "components/PdfExplorerComponent.h"
#include "components/PictureComponent.h"
#include "components/PictureExplorerComponent.h"
#include "components/TextScrollComponent.h"
#include "games/MetadataDescriptor.h"
#include "guis/Gui.h"
#include "views/PictogramCaches.h"

class GuiGameManual : public Gui
                      , private PdfExplorerComponent::Interface
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     * @paraml game game from witch to display manual or map
     */
    GuiGameManual(WindowManager& window, FileData& game);

    /*!
     * @brief Destructor
     * Update System view
     */
    ~GuiGameManual() override = default;


    /*!
     * @brief Manage inpout event to move/zoom the current pdf page
     * @param event Input event
     * @return True if the method consumed the event
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Override rendering to draw custom background
     * @param parentTrans parent transformation
     */
    void Render(const Transform4x4f& parentTrans) override;

    /*!
      * @brief Set help prompts
      * @param help Help structure to fill in
      * @return True
      */
    bool CollectHelpItems(Help& help) override;

  private:
    //! Current display
    enum class CurrentDisplay
    {
      ManualPdf,     //!< Manual in PDF format
      MapImage,      //!< Game maps, in any supported image format
      TipsAndTricks, //!< Game tips & tricks in text
    };

    //! Tips & Tricks
    class TipsAndTricks
    {
      public:
        /*!
         * @brief Constructor
         * @param language 2 lketter code Language
         * @param title Tip's title
         * @param text Tip's body
         */
        TipsAndTricks(const String& language, const String& title, const String& text, const String& preferedLanguage)
          : mLanguage(language)
          , mTitle(title)
          , mText(text)
        {
          mLanguage.LowerCase();
          if (mLanguage == preferedLanguage) mIndex = 0;
          else if (mLanguage == "en") mIndex = 1;
          else mIndex = 2;
        }

        //! Get Language
        [[nodiscard]] const String& Language() const { return mLanguage; }
        //! Get Title
        [[nodiscard]] const String& Title() const { return mTitle; }
        //! Get text
        [[nodiscard]] const String& Text() const { return mText; }
        //! Get index
        [[nodiscard]] int Index() const { return mIndex; }

      private:
        String mLanguage; //!< 2 letter language code
        String mTitle;    //!< Tip's title
        String mText;     //!< Tip's body
        int    mIndex;
    };

    //! Game
    FileData& mGame;

    //! Theme data
    const MenuThemeData& mMenuTheme;

    //! Title
    TextScrollComponent mTitle;
    //! Page indicator
    TextScrollComponent mPageCounter;
    //! Background
    NinePatchComponent mBackground;
    //! Grid
    Grid mGrid;

    //! Grid
    Grid mMarkdownGrid;
    //! Markdown title
    TextScrollComponent mMarkdownTitle;
    //! Markdown title
    PictureComponent mMarkdownFlag;
    //! Markdown text
    MarkdownText mMarkdown;

    //! Pdf viewer
    PdfExplorerComponent mPdf;
    //! Image explorer
    PictureExplorerComponent mImage;

    //! Tips & Tricks
    std::vector<TipsAndTricks> mTipsAndTricks;
    //! Maps, in scraping order
    std::vector<MetadataDescriptor::MapEntry> mMaps;
    //! Tips & tricks page nuimber
    int mCurrentTipsAndTricks;
    //! Currently displayed map index
    int mCurrentMap;
    //! Current adjusted display
    CurrentDisplay mCurrentDisplay;

    //! Sustained display
    static CurrentDisplay mRequestedDisplay;

    /*!
     * @brief Load and format tips and tricks
     */
    void LoadTipsAndTricks();

    /*!
     * @brief Fill text component with tips & tricks
     */
    void FillTipsAndTricks();

    /*!
     * @brief Fill image component with the current map
     */
    void FillMap();

    /*!
     * @brief Update page count
     * @param page Curren tpage
     * @param total Total page
     */
    void UpdatePage(int page, int total);

    /*!
     * @brief Display best manual according to mCurrentDisplay
     * @param force True to force component reloading
     */
    void DisplayCurrentManual(bool force);

    /*
     * PdfExplorerComponent::Interface
     */

    /*!
     * @brief Called when the current page changes
     * @param from Caller
     * @param currentPage New page
     * @param totalPages Total pages
     */
    void PdfPageChanged(PdfExplorerComponent& from, int currentPage, int totalPages) final { (void)from; UpdatePage(currentPage, totalPages); }
};


