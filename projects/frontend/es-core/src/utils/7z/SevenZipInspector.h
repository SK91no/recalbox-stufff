//
// Created by bkg2k on 01/02/25.
//
#include <utils/os/fs/Path.h>
#include <7zip/7zFile.h>
#include <7zip/7z.h>

class SevenZipInspector
{
  public:
    /*!
     * @brief Constructor
     * @param archive 7z archive file
     */
    explicit SevenZipInspector(const Path& archive);

    /*!
     * @brief Get list of all files from the archive
     * @return Path list
     */
    [[nodiscard]] const Path::PathList& AllFiles() const { return mPathList; }

    /*!
     * @brief Check if the archive is valid
     * @return
     */
    [[nodiscard]] bool IsValid() const { return !mError; }

  private:
    //! Internal buffer
    Byte mBuffer[1 << 14];
    //! Archive
    Path mArchivePath;
    //! List iof file in the archive
    Path::PathList mPathList;
    //! General error flag set if something fails while inspecting the archive
    bool mError;

    /*
     * 7z items
     */
    CFileInStream mFileInStream;
    CLookToRead2 mLookStream;
    CSzArEx mArchive;
    ISzAlloc mMainAlloc;
    ISzAlloc mTempAlloc;

    /*!
     * @brief Open the archive
     * @param archive Archive file
     * @return True if the operation is successful, false otherwise
     */
    bool OpenArchive(const Path& archive);

    //! Read all filenames from the archive & populate mPathList
    bool ReadAllFilenames();

    /*!
     * @brief Close the previously openened archive
     */
    void CloseArchive();
};
