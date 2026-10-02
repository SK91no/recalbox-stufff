//
// Created by bkg2k on 21/11/24.
//
#pragma once

#include "hardware/devices/storage/StorageDevices.h"
#include "hardware/devices/PhysicalDevices.h"
#include <guis/menus/MenuDefinition.h>

//! Forward declarations
class SystemData;
class FileData;
class ScriptDescriptor;
class ISoftPatchingNotifier;
class LobbyGame;
class IMetadataChanged;

class InheritableContext
{
  public:
    //! Always merge, never replace
    InheritableContext(const InheritableContext& source)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(0)
    {
      Merge(source);
    }
    //! Delete move constructor
    InheritableContext(InheritableContext&& source) = delete;

    //! Default constructor
    explicit InheritableContext(int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! System context
    explicit InheritableContext(SystemData* system, int index = 0)
      : mSystemData(system)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! System/game context
    InheritableContext(SystemData* system, FileData* game, IMetadataChanged* metadataInterface, int index = 0)
      : mSystemData(system)
      , mGameData(game)
      , mMetadata(metadataInterface)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Storage device context
    explicit InheritableContext(const StorageDevices::Device* device, int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(device)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Physical disk context
    explicit InheritableContext(const PhysicalDevices::Device* device, int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(device)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Script context
    explicit InheritableContext(const ScriptDescriptor* script, int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(script)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Softpatching interface context
    explicit InheritableContext(SystemData* system, FileData* game, ISoftPatchingNotifier* interface, int index = 0)
      : mSystemData(system)
      , mGameData(game)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(interface)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Lobby
    explicit InheritableContext(LobbyGame* lobbyGame, int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(lobbyGame)
      , mSetting(nullptr)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Setting
    explicit InheritableContext(const ItemDefinition::SettingDefinition* setting, int index = 0)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(setting)
      , mThemeOption(nullptr)
      , mIndex(index)
    {}

    //! Theme Option
    explicit InheritableContext(ThemeOption* themeOption)
      : mSystemData(nullptr)
      , mGameData(nullptr)
      , mMetadata(nullptr)
      , mDevice(nullptr)
      , mPhysicalDevice(nullptr)
      , mScript(nullptr)
      , mSoftpatching(nullptr)
      , mLobbyGame(nullptr)
      , mSetting(nullptr)
      , mThemeOption(themeOption)
      , mIndex(0)
    {}

    /*!
     * @brief Merge source data. Definied source data overwrite current data
     * @param source Source to merge
     * @return This
     */
    InheritableContext& Merge(const InheritableContext& source)
    {
      // Copy non-null source data only - Keep al others
      if (source.mSystemData != nullptr) mSystemData = source.mSystemData;
      if (source.mGameData != nullptr) mGameData = source.mGameData;
      if (source.mMetadata != nullptr) mMetadata = source.mMetadata;
      if (source.mDevice != nullptr) mDevice = source.mDevice;
      if (source.mPhysicalDevice != nullptr) mPhysicalDevice = source.mPhysicalDevice;
      if (source.mScript != nullptr) mScript = source.mScript;
      if (source.mSoftpatching != nullptr) mSoftpatching = source.mSoftpatching;
      if (source.mLobbyGame != nullptr) mLobbyGame = source.mLobbyGame;
      if (source.mSetting != nullptr) mSetting = source.mSetting;
      if (source.mThemeOption != nullptr) mThemeOption = source.mThemeOption;
      if (source.mIndex != 0) mIndex = source.mIndex;
      return *this;
    }

    /*
     * Setters
     */

    void SetIndex(int index) { mIndex = index; }

    /*
     * Getters
     */

    //! Has system?
    [[nodiscard]] bool HasSystem() const { return mSystemData != nullptr; }
    //! Has game
    [[nodiscard]] bool HasGame() const { return mGameData != nullptr; }
    //! Has game interface
    [[nodiscard]] bool HasGameInterface() const { return mMetadata != nullptr; }
    //! Has device
    [[nodiscard]] bool HasDevice() const { return mDevice != nullptr; }
    //! Has physical disk
    [[nodiscard]] bool HasPhysicalDevice() const { return mPhysicalDevice != nullptr; }
    //! Has script
    [[nodiscard]] bool HasScript() const { return mScript != nullptr; }
    //! Has softpatching
    [[nodiscard]] bool HasSoftpatching() const { return mSoftpatching != nullptr; }
    //! Has lobby game
    [[nodiscard]] bool HasLobbyGame() const { return mLobbyGame != nullptr; }
    //! Has Setting
    [[nodiscard]] bool HasSetting() const { return mSetting != nullptr; }
    //! Has Theme option
    [[nodiscard]] bool HasOption() const { return mThemeOption != nullptr; }

    //! Get system
    [[nodiscard]] int Index() const { return mIndex; }
    //! Get system
    [[nodiscard]] SystemData* System() const { return mSystemData; }
    //! Get game
    [[nodiscard]] FileData* Game() const { return mGameData; }
    //! Get game interface
    [[nodiscard]] IMetadataChanged* GameInterface() const { return mMetadata; }
    //! Get device
    [[nodiscard]] const StorageDevices::Device* Device() const { return mDevice; }
    //! Get physical disk
    [[nodiscard]] const PhysicalDevices::Device* PhysicalDevice() const { return mPhysicalDevice; }
    //! Get script
    [[nodiscard]] const ScriptDescriptor* Script() const { return mScript; }
    //! Get softpatching interface
    [[nodiscard]] ISoftPatchingNotifier* Softpatching() const { return mSoftpatching; }
    //! Get lobby game
    [[nodiscard]] LobbyGame* GameLobby() const { return mLobbyGame; }
    //! Get setting
    [[nodiscard]] const ItemDefinition::SettingDefinition* Setting() const { return mSetting; }
    //! Get Theme option
    [[nodiscard]] ThemeOption* Option() const { return mThemeOption; }

  private:
    //! System
    SystemData* mSystemData;
    //! Game
    FileData* mGameData;
    //! Game metadata interface
    IMetadataChanged* mMetadata;
    //! Storage device
    const StorageDevices::Device* mDevice;
    //! Physical disk
    const PhysicalDevices::Device* mPhysicalDevice;
    //! Storage device
    const ScriptDescriptor* mScript;
    //! Softpatching interface
    ISoftPatchingNotifier* mSoftpatching;
    //! Lobby
    LobbyGame* mLobbyGame;
    //! Menu settings
    const ItemDefinition::SettingDefinition* mSetting;
    //! Theme Option
    ThemeOption* mThemeOption;

    //! Free index
    int mIndex;
};
