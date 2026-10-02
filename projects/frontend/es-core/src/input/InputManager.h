#pragma once

#include <SDL2/SDL.h>
#include <utils/String.h>
#include <input/InputDevice.h>
#include <input/OrderedDevices.h>
#include <utils/os/fs/watching/FileSystemWatcher.h>
#include <utils/os/fs/watching/IFileSystemWatcherNotification.h>
#include "IInputChange.h"
#include <input/InputMapper.h>
#include "IKeyboardShortcut.h"
#include "utils/os/fs/watching/FileSystemWatcherHub.h"
#include "KeyboardLayoutMapper.h"

class WindowManager;

class InputManager : public StaticLifeCycleControler<InputManager>
                   , public IFileSystemWatcherNotification
                   , public SecuredFile::IValidationInterface
{
  public:
    /*!
     * @brief Default constructor
     */
    explicit InputManager(IKeyboardShortcut* shortcutInterface);

    /*!
     * @brief Default destructor
     */
    ~InputManager() override { ClearAllConfigurations(); };

    /*!
     * @brief Initialize the InputManager
     */
    void Initialize();

    /*!
     * Finalize the input manager and free all resources
     */
    void Finalize();

    /*!
     * @brief Refresh configurations. Reload all joysticks
     * @param window Main window
     */
    void Refresh(WindowManager* window, bool padplugged);

    //! Mapper accessor
    InputMapper& Mapper() { return mMapper; }

    /*!
     * Get number of initialized devices
     */
    [[nodiscard]] int DeviceCount() const { return (int)mIdToDevices.size(); }

    /*!
     * Get number of initialized devices
     */
    [[nodiscard]] int ConfigurableDeviceCount() const;

    /*!
     * @brief Parse an SDL event and generate an InputCompactEvent accordingly
     * @param window Window manager
     * @param ev SDL event
     * @return True if the resultEvent is valid, false otherwise
     */
    InputCompactEvent ManageSDLEvent(WindowManager* window, const SDL_Event& ev);

    /*!
     * @brief Get number of configured controllers, either manually or from Xml configuration file
     * @return Configured controllers count, not counting the keyboard
     */
    int ConfiguredControllersCount();

    /*!
     * @brief Get configuration path
     * @return Configuration path
     */
    static Path ConfigurationPath();

    /*!
     * @brief Write device configuration to Xml configuration file
     * @param device
     */
    void WriteDeviceXmlConfiguration(InputDevice& device);

    /*!
     * @brief Get device by index
     * @param index Device index
     * @return Device configuration
     */
    InputDevice& GetDeviceConfigurationFromIndex(int index) { return GetDeviceConfigurationFromId(mIndexToId[index]); }

    /*!
     * @brief Get device by SDL Identifier
     * @param deviceId Device identifier
     * @return Device configuration
     */
    InputDevice& GetDeviceConfigurationFromId(SDL_JoystickID deviceId);

    /*!
     * @brief Generate an ordered device list in function of player devices configurations
     * @return OrderedDevice object
     */
    OrderedDevices GetMappedDeviceList(const InputMapper& mapper);

    /*!
     * @brief Generate all player configurations into a single string
     * ready to be used in the configgen
     * @return Configuration string
     */
    String GetMappedDeviceListConfiguration(const InputMapper& mapper, bool lightgunGame);

    /*!
     * @brief Lookup Xml configuration for a particular device, looking for matching
     * guid and/or name
     * @param device Device to look for configuration
     * @return
     */
    bool LookupDeviceXmlConfiguration(InputDevice& device);

    /*!
     * @brief Tell whether an xml entry targets the given device: same guid and same capabilities.
     * Twins of a same model all match the very same entries.
     * @param device Device to check
     * @param item Xml entry
     * @return True if the entry may configure the device
     */
    static bool MatchesSignature(const InputDevice& device, pugi::xml_node item);

    /*!
     * @brief Load an xml entry into a device
     * @param device Device to configure
     * @param item Xml entry
     * @param portBound True when the entry is reserved to the port the device is plugged into
     * @return Always true, so that lookups may return it directly
     */
    static bool LoadDeviceXmlEntry(InputDevice& device, pugi::xml_node item, bool portBound);

    /*!
     * @brief Tell whether another connected device is indistinguishable from the given one
     * @param device Device to look a twin for
     * @return True if at least one twin is connected
     */
    [[nodiscard]] bool HasConnectedTwin(const InputDevice& device) const;

    /*!
     * @brief Log a detailed report of the raw input event
     * @param event Input event
     */
    static void LogRawEvent(const InputEvent& event);

    /*!
     * @brief Log a detailed report of the input compact event
     * @param event compact event
     */
    static void LogCompactEvent(const InputCompactEvent& event);

    /*!
     * @brief Add a new interface to call when a pad is added or removed
     * @param interface New interface
     */
    void AddNotificationInterface(IInputChange* interface);

    /*!
     * @brief Remove a notification interface
     * Does nothing if the given interface has not been previously added
     * @param interface Interface to remove
     */
    void RemoveNotificationInterface(IInputChange* interface);

    /*!
     * @brief Initialize SDL's joysticks
     */
    static void InitializeSDL2JoystickSystem();

    /*!
     * @brief Finalize SDL's joysticks
     */
    static void FinalizeSDL2JoystickSystem();

    /*!
     * @brief Check if there is a move in /dev/input
     */
    void WatchJoystickAddRemove(WindowManager* window);

    /*!
     * @brief Get device name by SDL Identifier
     * @param id Device identifier
     * @return Device name
     */
    String GetDeviceNameFromId(SDL_JoystickID id);

    /*!
     * @brief Get device name by SDL Identifier
     * @param id Device identifier
     * @return Device name
     */
    int GetDeviceIndexFromId(SDL_JoystickID id);

    /*!
     * @brief Add rebuilt SDL events to the 'to' array
     * @param to Event list
     */
    void GetRawEvents(Array<SDL_Event>& to);

    /*!
     * @brief Remap text input characters based on keyboard layout
     * @param text Input text from SDL_TEXTINPUT (assumes US layout)
     * @return Remapped text for the actual keyboard layout
     */
    String RemapTextInput(const char* text) const;

    /*!
     * @brief Check if using native SDL keyboard mapping (X11/Wayland)
     * @return True if using native SDL, false if custom remapping is needed
     */
    bool IsUsingNativeSDL() const { return mKeyboardLayoutMapper.IsUsingNativeSDL(); }

  private:
    //! Device list
    typedef std::vector<InputDevice> InputDeviceList;

    //! Index to SDL Identifiers
    SDL_JoystickID mIndexToId[Input::sMaxInputDevices];
    //! SDL Identifier to Joystick structures
    HashMap<SDL_JoystickID, SDL_Joystick*> mIdToSdlJoysticks;
    //! SDL Identifier to device configurations
    HashMap<SDL_JoystickID, InputDevice*> mIdToDevices;
    //! Default Keyboard
    InputDevice mKeyboard;
    //! Default Mousse
    InputDevice mMousse;

    //! Raw key events
    bool mScancodeStates[0x100];
    //! Previous raw key events
    bool mScancodePreviousStates[0x100];

    //! Notification interfaces
    Array<IInputChange*> mNotificationInterfaces;

    //! Input mapper (must be initialized after mNotificationInterfaces)
    InputMapper mMapper;

    //! watcher reader/dispatcher
    FileSystemWatcherHub mHub;
    //! /dev/input watcher
    FileSystemWatcher mWatcher;

    //! Shortcut interface
    IKeyboardShortcut* mShortcutInterface;

    //! Keyboard layout mapper
    KeyboardLayoutMapper mKeyboardLayoutMapper;

    //! AltGr key state tracking
    bool mAltGrPressed;

    //! Joystick change pendings
    bool mJoystickChangePending;
    //! joystick change pending - added or removed?
    bool mJoystickChangePendingRemoved;

    //! True when at least one RecalboxJVS light gun is plugged (refreshed with the device list)
    bool mHasJvsLightGun;

    /*!
     * @brief Detect whether a RecalboxJVS light gun is currently plugged
     * (the guns are mouse devices, invisible to SDL's joystick layer, so we scan /proc)
     * @return True if at least one "RecalboxJVS Gun" input device exists
     */
    static bool DetectJvsLightGun();

    /*!
     * @brief Load default keyboard configuration
     * Event if the user deletes es_input.cfg, he's still able to reconfigure using a keyboard
     */
    void LoadDefaultKeyboardConfiguration();

    /*!
     * @brief Get the initialization state
     * @return True if the manager is initialized
     */
    [[nodiscard]] bool IsInitialized() const { return !mIdToDevices.empty(); }

    /*!
     * @brief Get the GUID string of an SDL joystick
     * @param joystick SDL Joystick handle
     * @return GUID string
     */
    static String DeviceGUIDString(SDL_Joystick* joystick);

    /*!
     * @brief Clear all configurations
     */
    void ClearAllConfigurations();

    /*!
     * @brief Build current joystick list
     * @return Joystick list
     */
    std::vector<InputDevice> BuildCurrentDeviceList();

    /*!
     * @brief Remove from both lists, devices that are in both lists, keeping only unique devices in left or right lists
     * @param left Left list
     * @param right Right list
     */
    static void KeepDifferentPads(std::vector<InputDevice>& left, std::vector<InputDevice>& right);

    /*!
     * @brief Load all joystick and load configurations
     */
    void LoadAllJoysticksConfiguration(std::vector<InputDevice> previous, WindowManager* window, bool padplugged);

    /*!
     * @brief Load joystick configuration (by index)
     * @param index Joystick index from to 0 to available joysticks-1
     */
    void LoadJoystickConfiguration(int index);

    /*!
     * @brief Process an Axis SDL event and generate an InputCompactEvent accordingly
     * @param axis SDL event
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageAxisEvent(const SDL_JoyAxisEvent& axis);

    /*!
     * @brief Process a button SDL event and generate an InputCompactEvent accordingly
     * @param button SDL event
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageButtonEvent(const SDL_JoyButtonEvent& button);

    /*!
     * @brief Process a hat SDL event and generate an InputCompactEvent accordingly
     * @param hat SDL event
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageHatEvent(const SDL_JoyHatEvent& hat);

    /*!
     * @brief Process a keyboard SDL event and generate an InputCompactEvent accordingly
     * @param key SDL event
     * @param down True if the key is down
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageKeyEvent(const SDL_KeyboardEvent& key, bool down);

    /*!
     * @brief Process a mouse SDL event and generate an InputCompactEvent accordingly
     * @param button SDL event
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageMouseButtonEvent(const SDL_MouseButtonEvent& button, bool down);

    /*!
     * @brief Process a mouse wheel SDL event and generate an InputCompactEvent accordingly
     * @param wheel SDL event
     * @return InputCompactEvent filled with event information
     */
    InputCompactEvent ManageMouseWheelEvent(const SDL_MouseWheelEvent& wheel);

    /*
     * IFileSystemWatcherNotification implementation
     */

    /*!
     * @brief Receive file event
     * @param event Event type
     * @param path Path
     * @param time Timestamp
     */
    void FileSystemWatcherNotification(FileSystemWatcher& source, const Path& path, EventType event, const DateTime& time) override;

    /*
     * SecuredFile::IValidationInterface implementation
     */

    /*!
     * @brief Interface implementation must validate content
     * @param content File content
     * @return True if the content is valid, false otherwise
     */
    bool ValidateContent(const String& content) final;
};
