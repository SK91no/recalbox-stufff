//
// Created by bkg2k on 11/03/25.
//
#pragma once

class SystemData;

class ISystemScan
{
  public:
    //! Default vistrual constructor
    virtual ~ISystemScan() = default;

    /*!
     * @brief Initiate the system scan
     * @param system System being scanned
     */
    virtual void SystemScanStart(const SystemData& system) = 0;

    /*!
     * @brief Called once per file being scanned during all the scan process
     * @param system System being scanned
     * @param added Games added so far
     * @param removed Games removed so far
     */
    virtual void SystemScanProgress(const SystemData& system, const Path& path) = 0;

    /*!
     * @brief Called once per added file
     * @param system Target system
     * @param path Fullpath of added game
     * @param game Added game structure
     */
    virtual void SystemScanGameAdded(const SystemData& system, const Path& path, FileData& game) = 0;

    /*!
     * @brief Called once per removed file
     * @param system Target system
     * @param game Removed game structure
     */
    virtual void SystemScanGameRemoved(const SystemData& system, FileData& game) = 0;

    /*!
     * @brief Called at the end of scanning process
     * @param added Total added games
     * @param removed Total removed games
     */
    virtual void SystemScanEnd(SystemData&) = 0;
};