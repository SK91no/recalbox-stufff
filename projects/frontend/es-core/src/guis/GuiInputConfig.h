#pragma once

#include <guis/Gui.h>
#include <components/ComponentGrid.h>
#include <utils/locale/LocaleHelper.h>
#include <input/IInputChange.h>
#include "components/TextScrollComponent.h"
#include "components/StackablePictureComponent.h"
#include "components/MarkdownText.h"
#include <input/PadTypes.h>

#include "IDeviceConfigured.h"
#include "hardware/RGPProxy.h"

class GuiInputConfig : public Gui
                     , private IInputChange
{
  public:
    /*!
     * @param testOnly When true, the GUI starts directly in Testing state with
     * the device's existing bindings (loaded from es_input.cfg). No bindings are
     * cleared, no save is performed, and no callback is fired — long-press START
     * just closes the GUI. Used by "TEST A CONTROLLER" in the controller menu.
     */
    GuiInputConfig(WindowManager& window, InputDevice* target, PadTypes::Type targedType, IDeviceConfigured* callback, bool testOnly = false);
    ~GuiInputConfig() override;
    void onSizeChanged() override;
    bool CollectHelpItems(Help& help) override;

  private:
    //! Max axis
    static constexpr int sMaxAxis = 16;
    //! Max buttons
    static constexpr int sMaxButtons = 32;
    //! Max hats (mActiveHats packs 4 bits per hat in a 32-bit int)
    static constexpr int sMaxHats = 8;

    //! Cancel time
    static constexpr int sCancelTime = 5000; // 5000ms
    //! Cancel time before the cancel bar is shown
    static constexpr int sCancelShowTime = 1000; // 1000ms
    //! Display phase duration before advancing to the next chunk
    static constexpr int sDisplayPhaseTime = 1000; // 1000ms
    //! Long-press Start duration to commit & close in Testing
    static constexpr int sTestingLongPressMs = 1000; // 1000ms
    //! Wait phase blink full cycle (half on, half off)
    static constexpr int sWaitBlinkPeriod = 500; // 500ms
    //! Display phase intro animation: initial extra horizontal scaling on top of 1.0
    static constexpr float sDisplayIntroInitialCoef = 20.f;
    //! Display phase intro animation: per-frame decay multiplier
    static constexpr float sDisplayIntroDecay = 0.5f;
    //! Display phase intro animation: floor below which the coef snaps to 0
    static constexpr float sDisplayIntroFloor = 0.1f;

    struct PadImageItem
    {
      Path mPath;                      //!< Item image path
      String mHint;                    //!< Short hint, used at <=480p (e.g. "L1", "MENU")
      String mHintLong;                //!< Long hint, used at >480p (e.g. "L1 (Left Shoulder)", "MENU (Start)"). Empty falls back to mHint.
      InputEvent::EventType mPrefered; //!< Prefered input type
      bool mBelow;                     //!< True if the item requires the view from below, false for the above side
      bool mSkippable;                 //!< True if this item is skippable (no definition)
    };

    struct PadImageDescriptor
    {
      Path PadAboveFrame;                //!< Global Pad frame (Seen from above)
      Path PadBelowFrame;                //!< Global Pad frame (Seen from below)
      Path DPadNeutral;                  //!< Centered non-active digital pad (arcade pad)
      Path JoystickLeftNeutral;          //!< Centered non-active left joystick
      Path JoystickRightNeutral;         //!< Centered non-active right joystick
      Path DPadUpLeft;                   //!< Up/Left digital pad (arcade pad)
      Path DPadUpRight;                  //!< Up/Right digital pad (arcade pad)
      Path DPadDownLeft;                 //!< Down/Left digital pad (arcade pad)
      Path DPadDownRight;                //!< Down/Right digital pad (arcade pad)
      Path JoystickLeftUpLeft;           //!< Up/Left left joystick
      Path JoystickLeftUpRight;          //!< Up/Right left joystick
      Path JoystickLeftDownLeft;         //!< Down/Left left joystick
      Path JoystickLeftDownRight;        //!< Down/Right left joystick
      Path JoystickRightUpLeft;          //!< Up/Left right joystick
      Path JoystickRightUpRight;         //!< Up/Right right joystick
      Path JoystickRightDownLeft;        //!< Down/Left right joystick
      Path JoystickRightDownRight;       //!< Down/Right right joystick
      PadImageItem HatTop;               //!< Hat top ON
      PadImageItem HatRight;             //!< Hat right ON
      PadImageItem HatBottom;            //!< Hat bottom ON
      PadImageItem HatLeft;              //!< Hat left ON
      PadImageItem JoystickLeftUp;       //!< Left Joystick up ON
      PadImageItem JoystickLeftRight;    //!< Left Joystick right ON
      PadImageItem JoystickLeftBottom;   //!< Left Joystick bottom ON
      PadImageItem JoystickLeftLeft;     //!< Left Joystick left ON
      PadImageItem JoystickRightUp;      //!< Right Joystick up ON
      PadImageItem JoystickRightRight;   //!< Right Joystick right ON
      PadImageItem JoystickRightBottom;  //!< Right Joystick bottom ON
      PadImageItem JoystickRightLeft;    //!< Right Joystick left ON
      PadImageItem ButtonEast;              //!< East button (SNES A / PS circle) ON
      PadImageItem ButtonSouth;              //!< South button (SNES B / PS cross) ON
      PadImageItem ButtonEastPrime;              //!< EastPrime button (Megadrive C) ON
      PadImageItem ButtonNorth;              //!< North button (SNES X / PS triangle) ON
      PadImageItem ButtonWest;              //!< West button (SNES Y / PS square) ON
      PadImageItem ButtonNorthPrime;              //!< NorthPrime button (Megadrive Z) ON
      PadImageItem ButtonL1;             //!< L1 Shoulder ON
      PadImageItem ButtonR1;             //!< R1 Shoulder ON
      PadImageItem ButtonL2;             //!< L2 Trigger ON
      PadImageItem ButtonR2;             //!< R2 Trigger ON
      PadImageItem ButtonL3;             //!< L3 Button ON
      PadImageItem ButtonR3;             //!< R3 Button ON
      PadImageItem ButtonStart;          //!< Start button ON
      PadImageItem ButtonSelect;         //!< Select button ON
      PadImageItem ButtonHotkey;         //!< Hotkey button ON
    };

    //! Pad chunks in the configuration order
    enum class PadChunks
    {
      // Frames
      PadAboveFrame,         //!< Global Pad frame (Seen from above)
      PadBelowFrame,         //!< Global Pad frame (Seen from below)
      DPadNeutral,           //!< Centered non-active dpad
      JoystickLeftNeutral,   //!< Centered non-active left joystick
      JoystickRightNeutral,  //!< Centered non-active right joystick
      // Diagonal stick overlays — visual-only, derived from cardinals during Testing
      DPadUpLeft,            //!< Up/Left diagonal — DPAD (arcade pad)
      DPadUpRight,           //!< Up/Right diagonal — DPAD (arcade pad)
      DPadDownLeft,          //!< Down/Left diagonal — DPAD (arcade pad)
      DPadDownRight,         //!< Down/Right diagonal — DPAD (arcade pad)
      JoystickLeftUpLeft,    //!< Up/Left diagonal — left joystick
      JoystickLeftUpRight,   //!< Up/Right diagonal — left joystick
      JoystickLeftDownLeft,  //!< Down/Left diagonal — left joystick
      JoystickLeftDownRight, //!< Down/Right diagonal — left joystick
      JoystickRightUpLeft,   //!< Up/Left diagonal — right joystick
      JoystickRightUpRight,  //!< Up/Right diagonal — right joystick
      JoystickRightDownLeft, //!< Down/Left diagonal — right joystick
      JoystickRightDownRight,//!< Down/Right diagonal — right joystick
      // Items
      HatTop,               //!< Hat top ON
      HatRight,             //!< Hat right ON
      HatBottom,            //!< Hat bottom ON
      HatLeft,              //!< Hat left ON
      JoystickLeftUp,       //!< Left Joystick up ON
      JoystickLeftLeft,     //!< Left Joystick left ON
      JoystickLeftBottom,   //!< Left Joystick bottom ON
      JoystickLeftRight,    //!< Left Joystick right ON
      JoystickRightUp,      //!< Right Joystick up ON
      JoystickRightLeft,    //!< Right Joystick left ON
      JoystickRightBottom,  //!< Right Joystick bottom ON
      JoystickRightRight,   //!< Right Joystick right ON
      ButtonSouth,              //!< South button (SNES B / PS cross) ON
      ButtonEast,              //!< East button (SNES A / PS circle) ON
      ButtonEastPrime,              //!< EastPrime button (Megadrive C) ON
      ButtonWest,              //!< West button (SNES Y / PS square) ON
      ButtonNorth,              //!< North button (SNES X / PS triangle) ON
      ButtonNorthPrime,              //!< NorthPrime button (Megadrive Z) ON
      ButtonL1,             //!< L1 Shoulder ON
      ButtonR1,             //!< R1 Shoulder ON
      ButtonL2,             //!< L2 Trigger ON
      ButtonR2,             //!< R2 Trigger ON
      ButtonL3,             //!< L3 Button ON
      ButtonR3,             //!< R3 Button ON
      ButtonStart,          //!< Start button ON
      ButtonSelect,         //!< Select button ON
      ButtonHotkey,         //!< Hotkey button ON
      __Count               //!< Count item
    };

    //! Top-level state of the GUI
    enum class GlobalState
    {
      Configuring,   //!< Walk through PadChunks asking the user to map each item
      Testing,       //!< All chunks configured, real-time state visualization until validate
      WaitingProxy,  //!< Pad has no Select/Hotkey, R.G.P. filter has been pushed,
                     //!< polling for the proxy SDL device to appear before saving.
    };

    //! Sub-state inside Configuring
    enum class ConfigPhase
    {
      Wait,        //!< Slow blink current chunk between neutral and pressed
      Action,      //!< Current chunk shown pressed until the user releases everything
      Display,     //!< Capture overlay (black centre on white outline) for sDisplayPhaseTime ms, then advance
      AlreadyUsed, //!< Same timing as Display but red centre — input rejected because already mapped, falls back to Wait
    };

    /*!
     * @brief Lazily-built pad image table. The static map lives inside the
     * function so it's only initialized on first call rather than during app startup.
     * @return Reference to the global pad image table
     */
    static HashMap<PadTypes::Type, PadImageDescriptor>& GetPadDefinitions();

    /*
     * State machine
     */

    //! Switch sub-phase, reset phase elapsed timer
    void EnterPhase(ConfigPhase phase);
    //! Switch top-level state
    void EnterGlobalState(GlobalState state);
    /*!
     * @brief Move to the next chunk to configure, auto-skipping already-mapped
     * duplicates, transitioning to Testing once the last chunk is done.
     */
    void AdvanceChunk();
    /*!
     * @brief Step back to the previous "first-mapper" chunk (skipping axis
     * duplicates), unset its entry and re-prompt. No-op when already at the
     * first chunk.
     */
    void StepBack();

    //! Whether the current chunk is allowed to be skipped via DPad-Right
    [[nodiscard]] bool IsCurrentChunkSkippable() const;
    //! Resolve current chunk to its InputDevice::Entry
    [[nodiscard]] InputDevice::Entry CurrentEntry() const;
    //! Resolve current chunk to its PadImageItem (for prefered type, below flag, ...)
    [[nodiscard]] const PadImageItem* CurrentItem() const { return ItemForChunk(mCurrentChunk); }
    //! Resolve any chunk to its PadImageItem in the active pad descriptor; nullptr for visual-only chunks
    [[nodiscard]] const PadImageItem* ItemForChunk(PadChunks chunk) const;
    //! Resolve the entry already mapped to the given input to its PadImageItem (axis sign aware)
    [[nodiscard]] const PadImageItem* FindItemForInput(const InputEvent& input) const;

    //! Configuring-state input dispatcher (Wait/Action/Display)
    void HandleConfiguringInput();
    //! Testing-state input dispatcher (Hotkey+Start = save & close)
    void HandleTestingInput(const InputCompactEvent& event);

    /*!
     * @brief Whether the just-configured device should be proxified by R.G.P.
     * Returns true when neither Select nor Hotkey is mapped — in that case
     * START is meant to carry the special-action role and proxifying provides
     * the combos that emulate Select/Hotkey at the kernel layer.
     */
    [[nodiscard]] bool ShouldProxify() const;
    /*!
     * @brief Push the source pad name into the R.G.P. filter and switch to
     * GlobalState::WaitingProxy so Update() can poll for the proxy device.
     * Persistence of the bindings is deferred until the proxy is found.
     */
    void StartProxification();
    /*!
     * @brief Copy the bindings from the source InputDevice onto the proxy
     * InputDevice (already known to InputManager via SDL_JOYDEVICEADDED),
     * persist the proxy config to es_input.cfg, fire the callback, and Close().
     */
    void FinalizeProxification(int proxyIndex);
    //! Show an error popup, drop the in-memory bindings, Close() without saving.
    void ProxificationTimeout();

    /*!
     * @brief Try to assign the given input to the current chunk's entry.
     * Returns false if the input is already mapped to a different entry.
     * @param input Input event to assign
     * @return True if the assignment is successfull, false otherwise
     */
    bool TryAssignToCurrent(InputEvent input);

    /*!
     * @brief Refresh mSubtitle2 with the instruction string for the current state.
     * Caches mLastInstructions to avoid re-parsing markdown every frame.
     */
    void UpdateInstructions();
    //! Build the instruction text for the current state/phase
    [[nodiscard]] String BuildInstructions() const;

    /*!
     * @brief Refresh mCancel with the cancel-hold message + skip/back hints when applicable.
     * Caches mLastFooter to avoid resetting the TextScrollComponent scroll on every frame.
     * @param message Message to set in the footer
     */
    void UpdateFooter(const String& message);

    void Render(const Transform4x4f& parentTrans) override;
    bool ProcessInput(const InputCompactEvent& event) override;
    void Update(int deltaTime) override;

    //! Window background
    ComponentGrid mGrid;

    InputDevice mPreviousConfig;

    std::shared_ptr<TextScrollComponent> mTitle;
    std::shared_ptr<TextScrollComponent> mSubtitle1;
    //!< Phase instructions (2 lines, scrolls if overflow)
    std::shared_ptr<MarkdownText> mSubtitle2;
    std::shared_ptr<StackablePictureComponent> mPics;
    std::shared_ptr<TextScrollComponent> mCancel;

    InputDevice* mTargetDevice;
    IDeviceConfigured* mCallback;
    PadTypes::Type mTargetType;
    PadChunks mCurrentChunk;
    //! Top-level state
    GlobalState mGlobalState;
    //! Sub-state inside Configuring
    ConfigPhase mConfigPhase;
    //! Time elapsed in current phase (ms)
    int mPhaseElapsedMs;
    //! Time Start has been held in Testing (ms), -1 when not held
    int mStartHoldMs;
    //! Last captured event text, displayed in Display overlay
    String mLastCapturedText;
    //! Text animation
    float mDisplayWidthCoef;
    //! Last instruction string set on mSubtitle2 (cache to avoid re-parsing markdown)
    String mLastInstructions;

    //! Pad chunk to input entry mapper
    static const HashMap<PadChunks, InputDevice::Entry>& ChunkToEntryMap();

    std::vector<InputEvent> mEventList; //!< Recorded events
    int mActiveButtons;                 //!< Active (pressed) buttons
    int mActiveAxis;                    //!< Active (far enough from their original position) axis
    int mActiveHats;                    //!< Active (non zero) hats
    int mLastHatDirection[sMaxHats];    //!< Per-hat last pure direction (1/2/4/8), 0 when neutral
    int mButtonReferenceTime[sMaxButtons]; // Time record
    bool mNeutralPositionSet;           //!< Neutral position has been reached, we can start recording
    bool mFirstEventReceived;           //!< First event received flag
    bool mWaitNeutralSeen;              //!< Pad has been seen neutral while in Wait — gate before allowing Wait->Action
    bool mCancelled;
    bool mTestOnly;                     //!< Read-only test mode: no clear, no save, no callback (long-press START closes)

    // Snapshot of the kernel R.G.P. exit-combo toggles, taken at construction
    // so the destructor can restore them. Exit combos are disabled for the
    // whole lifetime of this GUI: pressing L3+R3 or holding START would
    // otherwise kill the parent emulator while the user is mid-config.
    RGPProxy::ExitCombosState mSavedExitCombos;

    // R.G.P. proxification state — populated when entering WaitingProxy.
    // The kernel grab makes the source SDL joystick disappear (REMOVED event);
    // PadsAddedOrRemoved checks mProxifyInProgress to avoid auto-Close().
    // The grab also triggers InputManager::Refresh which deletes every
    // InputDevice in mIdToDevices, so mTargetDevice becomes dangling — we
    // must snapshot the source bindings BEFORE pushing the kernel filter.
    bool mProxifyInProgress;       //!< True between StartProxification and Close/timeout
    unsigned int mSourceUid;       //!< Kernel R.G.P. UID stamped on the source pad
    String mSourceName;            //!< Source pad name pushed to the R.G.P. filter
    int mProxyWaitMs;              //!< Time elapsed in WaitingProxy state (ms)
    int mProxyPollMs;              //!< Throttle counter for FindProxyForUid polling
    InputEvent mSourceBindings[(int)InputDevice::Entry::__Count]; //!< Snapshot of source bindings
    int mSourceBindingsBits;       //!< Bitmask of valid entries in mSourceBindings

    //! Fill in all images in the multi-component
    void FillImage(PadImageDescriptor& descriptor);

    /*!
     * @brief Highlight the selected image if the 'on' flag is true.
     * Otherwise, display the appropriate frame
     */
    void HighlightImage();

    /*!
     * @brief Update mPics visibility for Testing state: every chunk lights up
     * based on the live pad state via its mapped binding.
     */
    void HighlightTesting();

    /*!
     * @brief Is this chunk currently activated by the live pad state?
     */
    [[nodiscard]] bool IsChunkActiveInTesting(PadChunks chunk) const;

    /*!
     * @brief Analyse and record raw event
     * @param raw raw event
     */
    void RecordRawInput(const InputEvent& raw);

    //! Check if the current pad is in neutral position
    [[nodiscard]] bool NeutralPosition() const { return (mActiveHats | mActiveButtons | mActiveAxis) == 0; }

    /*!
     * @brief Process all received events
     */
    void ProcessEvents();

    /*
     * IInputChange implementation
     */

    //! Refresh pad list. Suppressed during R.G.P. proxification — the kernel
    //! grab fires a REMOVED event we expect and must not act on.
    void PadsAddedOrRemoved(bool removed) override { if (removed && !mProxifyInProgress) Close(); };
};
