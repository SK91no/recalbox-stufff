//
// Created by bkg2k on 02/03/23.
//
#pragma once

#include <utils/os/system/Thread.h>
#include <utils/os/system/Signal.h>
#include <utils/storage/Queue.h>
#include <systems/SystemData.h>

class SystemDataCompleter : private Thread
{
  public:
    //! Destructor
    ~SystemDataCompleter() override;

    //! Push a new system to auto-hash
    void Push(SystemData* system);

    //! Push a new system to auto-hash
    void Push(FileData* system);

    //! Start completing data
    void Start();

    //! Application is about to quit - stop any processing asap
    void StopImmediately();

  private:
    //! Working signal
    Signal mSignal;
    //! Queue protector
    Mutex mLocker;
    //! System queue
    ::Queue<SystemData*> mSystemQueue;
    //! System queue
    ::Queue<FileData*> mFileQueue;

    /*!
     * @brief Check missing data and calculate them all for a whole system
     * @param system System to check for missing data
     */
    void CompleteSystemData(SystemData& system);

    /*!
     * @brief Check missing data and calculate them all for a single game
     * @param game Game to check for missing data
     */
    static void CompleteGameData(FileData& game);

    /*!
     * @brief Check & caclulate hash for the given game
     * @param game Game whose calculate the hash
     * @return True if the hash has been updated, false otherwise
     */
    static bool CheckHashFor(FileData& game);

    /*!
     * @brief Check & caclulate support type for the given game
     * @param game Game whose calculate the hash
     * @return True if the support has been updated, false otherwise
     */
    static bool CheckSupportTypeFor(FileData& game);

    /*!
     * @brief Check & caclulate hash for the given system
     * @param system System whose calculate all missing hashed
     * @return True if at least one hash has been updated, false otherwise
     */
    void CheckHashFor(SystemData& system);

    /*!
     * @brief Check & caclulate support type for the given game
     * @param system System whose calculate all missing support type
     * @return True if at least one support has been updated, false otherwise
     */
    void CheckSupportTypeFor(SystemData& game);

    /*
     * Thread implementation
     */

    /*!
     * @brief Break the thread asap
     */
    void Break() override { mSignal.Fire(); }

    /*!
     * @brief Process missing hashes in the queued systems
     */
    void Run() override;
};
