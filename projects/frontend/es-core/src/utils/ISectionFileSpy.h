//
// Created by bkg2k on 18/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#pragma once

#include <utils/String.h>

class ISectionFileSpy
{
  public:
    /*!
     * @brief Notify the file is being loaded
     * @param status True means the file or its fallback has been loaded successfuly
     */
    virtual void OnSectionFileLoad(bool status) = 0;

    /*!
     * @brief Notify the file is being saved
     */
    virtual void OnSectionFileSave() = 0;

    /*!
     * @brief Notify a section/key/value has been modified.
     * @param section Section
     * @param key Key
     * @param value Value
     */
    virtual void OnSectionFileModify(const String& section, const String& key, const String& value) = 0;

    /*!
     * @brief Notify a section/key has been deleted.
     * @param section Section
     * @param key Key
     */
    virtual void OnSectionFileDelete(const String& section, const String& key) = 0;

    /*!
     * @brief Notify a whole section has been deleted.
     * @param section Section
     */
    virtual void OnSectionFileDeletedSection(const String& section) = 0;
};
