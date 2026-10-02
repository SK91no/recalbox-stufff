//
// Created by Bkg2k on 04/02/2020.
//
#pragma once

class FileData;

class IFilter
{
  public:
    //! Destructor
    virtual ~IFilter() = default;

    /*!
     * @brief Filter a FileData entry
     * @param file FileData to filter
     * @return Return true to validate this entry, false to ignore it
     */
    virtual bool ApplyFilter(const FileData& file) = 0;

    /*!
     * @brief Called after the filter has been applied to a set of games
     */
    virtual void TearDown() { }
};