//
// Created by bkg2k on 15/07/23.
//
#pragma once

#include <utils/os/fs/Path.h>

class ISoftPatchingNotifier
{
  public:
    //! Virtual destructor
    virtual ~ISoftPatchingNotifier() = default;

    /*!
     * @brief Instruct the caller if the previous game launch has been using softpatched or original game
     * @return True iof the previous call was using softpatching, false otherwise
     */
    virtual bool PreviousLaunchWasUsingSoftpathing() = 0;

    /*!
     * @brief Notify the soft patching is disabled
     */
    virtual void SoftPathingDisabled() = 0;

    /*!
     * @brief Notify a path has been selected
     * @param path Selected patch's path
     */
    virtual void SoftPatchingSelected(const Path& path)= 0;
};