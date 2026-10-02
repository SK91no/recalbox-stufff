//
// Created by digitalLumberjack on 18/11/2025.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/String.h>

class IRemotePlaylistNotification
{
  public:
    //! Required default destructor
    virtual ~IRemotePlaylistNotification() = default;

    /*!
     * @brief Notify of the availability of a remote music/playlist
     */
    virtual void PlaylistReady() = 0;
};
