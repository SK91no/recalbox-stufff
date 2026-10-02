//
// Created by bkg2k on 17/04/2022.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

class ICrt
{
  public:
    //! Destructor
    virtual ~ICrt() = default;

    /*!
     * @brief Main video outputs to CRT device
     * @return True if the main video outputs to CRT device, false otherwise
     */
    virtual bool HasCrt() = 0;

    /*!
     * @brief Get real CRT width (may use high resolution width, 1920)
     * @return Real CRT width
     */
    virtual int CrtWidth() = 0;

    /*!
     * @brief Get virtual CRT width the front end will work with (320 for high-resolution width)
     * @return virtual CRT width
     */
    virtual int CrtVirtualWidth() = 0;

    /*!
     * @brief Get real CRT height
     * @return Real CRT height
     */
    virtual int CrtHeight() = 0;

    /*!
     * @brief Get virtual CRT height (for now, == height)
     * @return virtual CRT height
     */
    virtual int CrtVirtualHeight() = 0;
};