//
// Created by bkg2k on 19/03/2026.
//
#pragma once

class IMusicCallback
{
  public:
    //! Default destructor
    virtual ~IMusicCallback() = default;

    /*!
     * @brief Start playing a musuc
     * @param name File name w/o extension
     */
    virtual void MusicStart(const String& name) = 0;

    /*!
     * @brief Called when the music ends
     */
    virtual void MusicComplete() = 0;
};