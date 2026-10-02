//
// Created by bkg2k on 23/03/25.
//
#pragma once

// Forward declaration
class SystemData;

class ISystemAlteredInterface
{
  public:
    //! Default destructor
    virtual ~ISystemAlteredInterface() = default;

    /*!
     * @brief Instruct the callee that a system gamelist has been altered
     * @param system First system whose gamelist has been altered
     */
    virtual void SystemGamelistAltered(SystemData* system) = 0;

    /*!
     * @brief Instruct the callee that a system rom folder has been altered
     * @param system First system whose roms have been altered
     */
    virtual void SystemRomFolderAltered(SystemData* system) = 0;
};