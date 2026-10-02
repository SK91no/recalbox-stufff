//
// Classic on-screen virtual keyboard (alternative to GuiArcadeVirtualKeyboard).
// Occupies the bottom half of the screen; text edit area sits above.
//
#pragma once

#include <guis/Gui.h>
#include <guis/IVirtualKeyboardInterface.h>
#include <guis/VKLayout.h>
#include <guis/VirtualKeyboardTheme.h>
#include <themes/IThemeSwitchable.h>
#include <utils/datetime/DateTime.h>
#include <Renderer.h>
#include <rendering/fonts/Font.h>

class GuiClassicVirtualKeyboard : public Gui
                                , public IVirtualKeyboardBase
                                , public IThemeSwitchable
{
  private:
    //! Modifier state machine (identical for Shift/Ctrl/Alt/AltGr)
    enum class ModState : unsigned char
    {
      Off,    //!< Not active
      Once,   //!< Active for the next character only (short tap)
      Locked, //!< Active until next short tap (set by long press)
    };

    enum class Direction : unsigned char
    {
      None, Left, Right,
    };

    //! Long-press threshold to switch a modifier from Once → Locked
    static constexpr int sModifierLockHoldMs = 750;

    //! Cursor auto-repeat delays (for text cursor inside the editor)
    static constexpr int sCursorRepeatStartMs = 350;
    static constexpr int sCursorRepeatTickMs  = 80;

    //! Grid-selection auto-repeat delays (D-pad held to move key by key)
    static constexpr int sGridRepeatStartMs = 400;
    static constexpr int sGridRepeatTickMs  = 90;

    //! Bottom screen fraction reserved so the help bar stays visible underneath
    static constexpr float sHelpBarReservedRatio = 0.06f;

    //! Fixed font height for 240p screens (pixels)
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
    static constexpr unsigned int sKeyModifierOnce       = 0x4488FFFFu;
    static constexpr unsigned int sKeyModifierLocked     = 0x224488FFu;
    static constexpr unsigned int sTitleTextColor        = 0xFFFF80FFu;
    static constexpr unsigned int sEditableTextColor     = 0xFFFFFFFFu;

    /*
     * Edited text state (mirrors GuiArcadeVirtualKeyboard's layout)
     */

    std::vector<unsigned int> mText;
    int mCursor;
    float mOffsetInPixel;
    DateTime mCursorBlinkReference;

    /*
     * Keyboard state
     */

    const VKLayout* mLayout;
    int mSelRow;
    int mSelCol;

    //! Sticky horizontal anchor (natural keyboard X) for vertical navigation, so
    //  up/down keep landing on the same column even across wide keys (Space).
    //  Negative when unset; reset by any horizontal move.
    float mDesiredCenterX;

    //! Modifier states + per-modifier hold timers (ms). -1 if not held.
    ModState mShift;
    ModState mCtrl;
    ModState mAlt;
    ModState mAltGr;
    int mShiftHoldMs;
    int mCtrlHoldMs;
    int mAltHoldMs;
    int mAltGrHoldMs;
    //! True if the modifier press already promoted to Locked during this hold,
    //  so the matching release must not flip state back.
    bool mShiftLockedDuringHold;
    bool mCtrlLockedDuringHold;
    bool mAltLockedDuringHold;
    bool mAltGrLockedDuringHold;

    //! Horizontal scroll offset (pixels) for low-res screens
    float mScrollX;

    //! Auto-repeat for text cursor while VK keyboard arrows are pressed
    Direction mTextCursorDirection;
    int mTextCursorRepeatTimerMs;

    //! Auto-repeat for grid selection while a D-pad direction is held.
    //  mNavRepeatDRow/DCol hold the active step (0/0 when idle).
    int mNavRepeatDRow;
    int mNavRepeatDCol;
    int mNavRepeatTimerMs;

    //! Valid (A) button currently held — inhibit grid selection movement
    bool mValidHeld;

    //! First-frame init flag (to fire VirtualKeyboardShow callback once)
    bool mShown;

    /*
     * Callbacks + filtering
     */

    IVirtualKeyboardInterface* mValidateCallback;
    VKCharFilter mFilter;

    //! Colors (seeded with the defaults below, overridden by the theme "keyboard" element)
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

    //! Cached layout metrics computed by RecomputeLayout()
    float mKeyboardTop;
    float mKeyboardHeight;
    float mUnitWidth;
    float mKeyHeight;
    float mKeyboardNaturalWidth;
    float mKeyboardLeftWhenFits;
    bool  mKeyboardOverflows;
    struct Rect { float x, y, w, h; };
    Rect mInnerEditor;
    Rect mOuterEditor;

    /*
     * Helpers
     */

    [[nodiscard]] bool IsCharAllowed(unsigned int unicode) const;
    [[nodiscard]] bool ShiftActive() const { return mShift != ModState::Off; }
    [[nodiscard]] bool AltGrActive() const { return mAltGr != ModState::Off; }
    [[nodiscard]] unsigned int ResolveChar(const VKKey& key) const;
    [[nodiscard]] unsigned int GetKeyDisplayChar(const VKKey& key) const;

    void LoadFonts();
    void RecomputeLayout();
    void ClampSelection();
    void MoveGridSelection(int dRow, int dCol);
    void StartGridRepeat(int dRow, int dCol);
    void StopGridRepeat();
    void EnsureSelectedKeyVisible();

    void PressSelectedKey();
    void AddCharacter(unsigned int unicode);
    void Backspace();
    void ValidateEditor();
    void ConsumeOnceModifiers();

    void AdjustCursor(int cursorextend = 0);
    void ResetCursorBlink();

    void TickModifier(ModState& state, int& holdMs, bool& lockedDuringHold,
                      bool pressed, int deltaTime);
    void ModifierShortTap(ModState& state);

    Rect KeyRectangle(int row, int col) const;
    [[nodiscard]] unsigned int KeyBackgroundColor(const VKKey& key, ModState modState, bool selected) const;

    void RenderTextBox();
    void RenderEditedString();
    void RenderKeyboard();
    void RenderKey(const VKKey& key, const Rect& rect, bool selected);

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
     * IThemeSwitchable implementation — keep the keyboard styled live when the
     * theme (or its options) change at runtime, e.g. from the web manager.
     */
    [[nodiscard]] SystemData* SystemTheme() const override { return nullptr; } //!< Use the global/main theme
    void SwitchToTheme(const ThemeData& theme, bool refreshOnly, IThemeSwitchTick* interface) override;

  public:
    GuiClassicVirtualKeyboard(WindowManager& window, const String& title, const String& initValue,
                              IVirtualKeyboardInterface* okCallback,
                              VKCharFilter filter = VKCharFilter::All,
                              const String& view = String::Empty);

    ~GuiClassicVirtualKeyboard() override { SDL_StopTextInput(); }

    [[nodiscard]] String Text() const { return String::JoinUnicodes(mText); }
};
