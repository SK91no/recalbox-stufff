//
// Created by bkg2k on 30/03/2020.
//
#pragma once

#include <web/server/IRouter.h>
#include <web/server/handlers/providers/SysInfos.h>
#include <bios/BiosManager.h>
#include <mqtt/MqttClient.h>
#include <web/server/handlers/providers/EmulationStationWatcher.h>
#include "systems/SystemManager.h"
#include "systems/SystemData.h"

class RequestHandler : public IRouter
{
  private:
    //! Bios Manager
    BiosManager& mBiosManager;
    //! SystemManager reference
    SystemManager& mSystemManager;
    //! System information provider
    SysInfos mSysInfos;
    //! Event watcher
    EmulationStationWatcher mWatcher;

    //! WWW root
    Path mWWWRoot;
    //! WWW default file
    String mDefaultFile;

    /*!
     * @brief Decode the given string using base64 algorythm
     * @param base64 String to decode
     * @return Decoded string
     */
    static String Decode64(const String& base64);

  public:
    RequestHandler(const String& wwwRoot, const String& defaultFile, SystemManager& systemManager)
      : mBiosManager(BiosManager::Instance())
      , mSystemManager(systemManager)
      , mWWWRoot(wwwRoot)
      , mDefaultFile(defaultFile)
    {
    }

    //! default virtual destructor
    ~RequestHandler() override = default;

    /*!
     * @brief Handle files
     * @param request Request object
     * @param response Response object
     */
    void FileServer(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET version
     * @param request Request object
     * @param response Response object
     */
    void Versions(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET Architecture
     * @param request Request object
     * @param response Response object
     */
    void Architecture(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET version
     * @param request Request object
     * @param response Response object
     */
    void SystemInfo(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET version
     * @param request Request object
     * @param response Response object
     */
    void StorageInfo(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET version
     * @param request Request object
     * @param response Response object
     */
    void BiosDownload(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET version
     * @param request Request object
     * @param response Response object
     */
    void BiosUpload(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get array of all system's bios
     * @param request Request object
     * @param response Response object
     */
    void BiosGetAll(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get bios for the given system
     * @param request Request object
     * @param response Response object
     */
    void BiosGetSystem(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get all systems
     * @param request Request object
     * @param response Response object
     */
    void SystemsGetAll(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get all active  systems
     * @param request Request object
     * @param response Response object
     */
    void SystemsGetActives(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system console svg for the given system and region
     * @param request Request object
     * @param response Response object
     */
    void SystemsResourceGetConsole(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system controller svg for the given system and region
     * @param request Request object
     * @param response Response object
     */
    void SystemsResourceGetController(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system controls svg for the given system and region
     * @param request Request object
     * @param response Response object
     */
    void SystemsResourceGetControls(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system game support svg for the given system and region
     * @param request Request object
     * @param response Response object
     */
    void SystemsResourceGetGame(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system logo svg for the given system and region
     * @param request Request object
     * @param response Response object
     */
    void SystemsResourceGetLogo(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get configuration keys
     * @param request Request object
     * @param response Response object
     */
    void ConfigurationGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle POST set configuration keys
     * @param request Request object
     * @param response Response object
     */
    void ConfigurationSet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle OPTIONS get configuration keys, type and validators
     * @param request Request object
     * @param response Response object
     */
    void ConfigurationOptions(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle DELETE delete configuration keys
     * @param request Request object
     * @param response Response object
     */
    void ConfigurationDelete(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get system configuration keys
     * @param request Request object
     * @param response Response object
     */
    void SystemConfigurationGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle POST set system configuration keys
     * @param request Request object
     * @param response Response object
     */
    void SystemConfigurationSet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle OPTIONS get system configuration keys, type and validators
     * @param request Request object
     * @param response Response object
     */
    void SystemConfigurationOptions(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle DELETE delete system configuration keys
     * @param request Request object
     * @param response Response object
     */
    void SystemConfigurationDelete(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle OPTIONS get media options
     * @param request Request object
     * @param response Response object
     */
    void MediaOptions(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get media list
     * @param request Request object
     * @param response Response object
     */
    void MediaGetList(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle DELETE delete media
     * @param request Request object
     * @param response Response object
     */
    void MediaDelete(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle POST create screenshot
     * @param request Request object
     * @param response Response object
     */
    void MediaTakeScreenshot(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get game media
     * @param request Request object
     * @param response Response object
     */
    void MediaGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get screenshot media
     * @param request Request object
     * @param response Response object
     */
    void MediaGetScreenshot(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms total count
     * @param request Request object
     * @param response Response object
     */
    void RomsGetTotal(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get system roms
     * @param request Request object
     * @param response Response object
     */
    void RomsGetList(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle DELETE to delete rom
     * @param request Request object
     * @param response Response object
     */
    void RomsDelete(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's information
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetInfo(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's image
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetImage(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's box
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetBox(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's video
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetVideo(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's map
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetMap(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET to get roms's manual
     * @param request Request object
     * @param response Response object
     */
    void MetadataGetManual(const Rest::Request& request, Http::ResponseWriter response) override;

    /*
     * Status
     */

    /*!
     * @brief Handle GET to get current system/game status
     * @param request Request object
     * @param response Response object
     */
    void StatusGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*
     * Themes
     */

    /*!
     * @brief Handle GET get theme system view value
     * @param request
     * @param response
     */
    void ThemeSystemViewGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get theme menu set value
     * @param request
     * @param response
     */
    void ThemeMenuSetGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get theme icon set value
     * @param request
     * @param response
     */
    void ThemeIconSetGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get theme gamelist view value
     * @param request
     * @param response
     */
    void ThemeGamelistViewGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get theme color set value
     * @param request
     * @param response
     */
    void ThemeColorSetGet(const Rest::Request& request, Http::ResponseWriter response) override;

    /*!
     * @brief Handle GET get theme gameclip view value
     * @param request
     * @param response
     */
    void ThemeGameclipViewGet(const Rest::Request& request, Http::ResponseWriter response) override;
};
