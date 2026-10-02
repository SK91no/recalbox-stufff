//
// Created by bkg2k on 10/03/24.
//
#pragma once

#include <utils/os/fs/Path.h>

/*!
 * @brief Load & save file in a secure way, protecting it from unexpected power off
 * and some kind of corruption & FS errors
 */
class SecuredFile
{
  public:
    /*!
     * @brief Manual validation interface
     */
    class IValidationInterface
    {
      public:
        //! Default destructoir
        virtual ~IValidationInterface() = default;

        /*!
         * @brief Interface implementation must validate content
         * @param content File content
         * @return True if the content is valid, false otherwise
         */
        virtual bool ValidateContent(const String& content) = 0;
    };

    /*!
     * @brief Load a file in a secure way
     * @param path Original path
     * @param fallback Fallback path
     * @param content Output content
     * @param logname Name for log records
     * @param autoBackup If true, the file is backuped before being replaced
     * @param validator Validation interface
     * @return True if the content has been loaded successfully, false otherwise
     */
    static bool LoadSecuredFile(bool mandatory, const Path& path, const Path& fallback, [[out]] String& content, const String& logname, bool autoBackup, IValidationInterface* validator);

    /*!
     * @brief Load a file in a secure way
     * @param path Original path
     * @param fallback Fallback path
     * @param content Output content
     * @param logname Name for log records
     * @param autoBackup If true, the file is backuped before being replaced
     * @return True if the content has been loaded successfully, false otherwise
     */
    static bool LoadSecuredFile(bool mandatory, const Path& path, const Path& fallback, [[out]] String& content, const String& logname, bool autoBackup)
    {
      return LoadSecuredFile(mandatory, path, fallback, content, logname, autoBackup, nullptr);
    }

    /*!
     * @brief Save a file in a secure way
     * @param path Destination path
     * @param content File ocntent
     * @param logname Name for log records
     * @param autoBackup If true, the file has a potentiel backup file
     * @param validator Validation interface
     * @return True if the content has been saved successfully, false otherwise
     */
    static bool SaveSecuredFile(const Path& path, const String& content, const String& logname, bool autoBackup, IValidationInterface* validator);

    /*!
     * @brief Save a file in a secure way
     * @param path Destination path
     * @param content File ocntent
     * @param logname Name for log records
     * @param autoBackup If true, the file has a potentiel backup file
     * @return True if the content has been saved successfully, false otherwise
     */
    static bool SaveSecuredFile(const Path& path, const String& content, const String& logname, bool autoBackup)
    {
      return SaveSecuredFile(path, content, logname, autoBackup, nullptr);
    }

  private:
    //! Temporary extension
    static constexpr const char* sTemporaryExtension = ".intermediate-tmp";

    /*!
     * @brief Load and check a single file
     * @param path Path to load & check
     * @param logname Name for log records
     * @param validator Validation interface
     * @return True if the content has been loaded successfully, false otherwise
     */
    static bool Load(const Path& path, const String& logname, [[out]] String& content, IValidationInterface* validator);
};

