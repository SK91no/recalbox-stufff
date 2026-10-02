//
// Simplified on-screen virtual keyboard: a real keyboard split into pages
// (lowercase, uppercase, symbols...) switched with L1/R1 like the wheel's
// charsets. No modifier keys: everything is reachable through the pages.
//
#pragma once

#include <vector>
#include <guis/Gui.h>
#include <guis/IVirtualKeyboardInterface.h>
#include <guis/VKLayout.h>
#include <guis/VirtualKeyboardTheme.h>
#include <themes/IThemeSwitchable.h>
#include <utils/datetime/DateTime.h>
#include <Renderer.h>
#include <rendering/fonts/Font.h>

class GuiSimplifiedVirtualKeyboard : public Gui
                                   , public IVirtualKeyboardBase
                                   , public IThemeSwitchable
{
  private:
    //! What a key does
    enum class KeyKind : unsigned char
    {
      Char,       //!< Emits its codepoint
      Backspace,  //!< Delete char left of cursor
      Enter,      //!< Validate & close
      Space,      //!< Emits ' '
      Left,       //!< Move text cursor left
      Right,      //!< Move text cursor right
    };

    enum class Direction : unsigned char { None, Left, Right };

    //! A single on-screen key (built at construction time)
    struct Key
    {
      KeyKind kind;       //!< Behaviour
      unsigned int code;  //!< Codepoint for Char keys (0 otherwise)
      float width;        //!< Width in unit cells (1.0 = one cell)
      String label;       //!< Label for non-Char keys
    };
    using Row  = std::vector<Key>;
    using Page = std::vector<Row>;

    //! Cursor auto-repeat delays (text cursor, L2/R2)
    static constexpr int sCursorRepeatStartMs = 350;
    static constexpr int sCursorRepeatTickMs  = 80;
    //! Grid-selection auto-repeat delays (D-pad held to move key by key)
    static constexpr int sGridRepeatStartMs = 400;
    static constexpr int sGridRepeatTickMs  = 90;

    //! Bottom screen fraction reserved so the help bar stays visible underneath
    static constexpr float sHelpBarReservedRatio = 0.06f;
    //! Fixed key font height for 240p screens (pixels)
    static constexpr int sLowResKeyFontPx = 7;

    //! UI ratios (relative to screen min dimension)
    static constexpr float sTitleFontRatio = 0.045f;
    static constexpr float sTextFontRatio  = 0.085f;
    static constexpr float sKeyFontRatio   = 0.05f;

    //! Colors
    static constexpr unsigned int sKeyBackground         = 0x303030F0u;
    static constexpr unsigned int sKeySelectedBackground = 0xC02020FFu;
    static constexpr unsigned int sKeyText               = 0xFFFFFFFFu;
    static constexpr unsigned int sKeyDisabledText       = 0x707070FFu;
    static constexpr unsigned int sTitleTextColor        = 0xFFFF80FFu;
    static constexpr unsigned int sEditableTextColor     = 0xFFFFFFFFu;

    //! Number of symbol columns per symbol row
    static constexpr int sSymbolCols = 9;

    /*
     * Edited text state (mirrors GuiClassicVirtualKeyboard)
     */
    std::vector<unsigned int> mText;
    int mCursor;
    float mOffsetInPixel;
    DateTime mCursorBlinkReference;

    /*
     * Pages + selection
     */
    std::vector<Page> mPages;
    int mPage;
    int mSelRow;
    int mSelCol;
    //! Sticky horizontal anchor for vertical navigation (negative when unset)
    float mDesiredCenterX;
    //! Sizing helpers computed once from the pages
    int   mMaxRowCount;
    float mMaxUnitsX100;

    //! Text cursor auto-repeat (L2/R2)
    Direction mTextCursorDirection;
    int mTextCursorRepeatTimerMs;
    //! Grid selection auto-repeat (0/0 when idle)
    int mNavRepeatDRow;
    int mNavRepeatDCol;
    int mNavRepeatTimerMs;

    bool mShown;

    /*
     * Callbacks + filtering
     */
    IVirtualKeyboardInterface* mValidateCallback;
    VKCharFilter mFilter;

    //! Colors (defaults overridden by the theme "keyboard" element of the view)
    VirtualKeyboardTheme mColors;
    //! Theme view name this keyboard is shown over (kept for live theme re-apply)
    String mView;

    /*
     * Fonts + cached metrics
     */
    NewFont* mKeyFont;
    NewFont* mTitleFont;
    NewFont* mTextFont;
    String mTitle;

    float mKeyboardTop;
    float mKeyboardHeight;
    float mUnitWidth;
    float mKeyHeight;
    float mKeyboardNaturalWidth;
    float mKeyboardLeft;
    struct Rect { float x, y, w, h; };
    Rect mInnerEditor;
    Rect mOuterEditor;

    /*
     * Page building
     */
    void BuildPages();
    static Row MakeDigitRow();
    void AddFunctionRow(Page& page) const;
    void AppendSymbolPages(const std::vector<unsigned int>& symbols);
    void ComputeSizingMetrics();

    /*
     * Helpers
     */
    [[nodiscard]] bool IsCharAllowed(unsigned int unicode) const;
    [[nodiscard]] const Page& CurrentPage() const { return mPages[mPage]; }
    [[nodiscard]] String PageLabel(int page) const;

    void LoadFonts();
    void RecomputeLayout();
    void ClampSelection();
    void MoveGridSelection(int dRow, int dCol);
    void StartGridRepeat(int dRow, int dCol);
    void StopGridRepeat();
    void ChangePage(int delta);

    void PressSelectedKey();
    void AddCharacter(unsigned int unicode);
    void Backspace();
    void DeleteAtCursor();
    void ValidateEditor();
    void MoveTextCursor(int delta);

    void AdjustCursor(int cursorextend = 0);
    void ResetCursorBlink();

    [[nodiscard]] Rect KeyRectangle(int row, int col) const;

    void RenderTextBox();
    void RenderEditedString();
    void RenderKeyboard();
    void RenderKey(const Key& key, const Rect& rect, bool selected);

    [[nodiscard]] static bool IsLowRes() { return Renderer::Instance().Is240p(); }
    [[nodiscard]] static float GetFontSize(float ratio);

  protected:
    void textInput(const char* text) override;
    bool ProcessInput(const InputCompactEvent& event) override;
    void Update(int deltaTime) override;
    void Render(const Transform4x4f& parentTrans) override;
    bool CollectHelpItems(Help& help) override;
    [[nodiscard]] bool IsOverlay() const override { return true; }
    //! Render under the help system so the help bar stays clear over the keyboard
    [[nodiscard]] bool MustRenderOverHelpSystem() const override { return false; }

    /*
     * IThemeSwitchable implementation — live re-style on runtime theme changes.
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return nullptr; } //!< Use the global/main theme
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

  public:
    GuiSimplifiedVirtualKeyboard(WindowManager& window, const String& title, const String& initValue,
                                 IVirtualKeyboardInterface* okCallback,
                                 VKCharFilter filter = VKCharFilter::All,
                                 const String& view = String::Empty);

    ~GuiSimplifiedVirtualKeyboard() override { SDL_StopTextInput(); }

    [[nodiscard]] String Text() const { return String::JoinUnicodes(mText); }
};
