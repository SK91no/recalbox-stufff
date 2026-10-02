//
// Created by bkg2k on 01/02/25.
//

#include "SevenZipInspector.h"
#include "7zip/7zCrc.h"
#include <utils/Log.h>

static void *Alloc(const ISzAlloc* p, size_t size)
{
  (void)p;
  if (size == 0) return nullptr;
  return malloc(size);
}

void Free(const ISzAlloc* p, void *address)
{
  (void)p;
  free(address);
}

SevenZipInspector::SevenZipInspector(const Path& archive)
  : mBuffer()
  , mArchivePath(archive)
  , mError(false)
  , mFileInStream()
  , mLookStream()
  , mArchive()
  , mMainAlloc()
  , mTempAlloc()
{
  memset(mBuffer, 0, sizeof(mBuffer));
  memset(&mArchive, 0, sizeof(mArchive));
  memset(&mLookStream, 0, sizeof(mLookStream));
  memset(&mFileInStream, 0, sizeof(mFileInStream));
  memset(&mMainAlloc, 0, sizeof(mMainAlloc));
  memset(&mTempAlloc, 0, sizeof(mTempAlloc));

  if (OpenArchive(archive))
    ReadAllFilenames();
  CloseArchive();
}

bool SevenZipInspector::OpenArchive(const Path& archive)
{
  // Init alloc functions
  mMainAlloc.Alloc = &Alloc;
  mMainAlloc.Free = &Free;
  mTempAlloc.Alloc = &Alloc;
  mTempAlloc.Free = & Free;

  // Open archive file
  if (InFile_Open(&mFileInStream.file, archive.ToChars()) != 0)
  {
    { LOG(LogError) << "Unable to open 7z archive " << archive; }
    return false;
  }

  mLookStream.bufSize   = sizeof(mBuffer);
  mLookStream.buf       = mBuffer;

  // Initialize streams
  FileInStream_CreateVTable(&mFileInStream);
  LookToRead2_CreateVTable(&mLookStream, False);
  mLookStream.realStream = &mFileInStream.vt;
  LookToRead2_Init(&mLookStream);

  // CRC table
  CrcGenerateTable();
  // Initialize 7z database
  SzArEx_Init(&mArchive);

  // Open archive
  if (int err = SzArEx_Open(&mArchive, &mLookStream.vt, &mMainAlloc, &mTempAlloc); err != SZ_OK)
  {
    { LOG(LogError) << "Unable to read 7z archive contents of " << archive << ". Error number: " << err; }
    return false;
  }

  return true;
}

bool SevenZipInspector::ReadAllFilenames()
{
  // Browse and get filenames
  unsigned short Unicode16[8192];
  for(int i = (int)mArchive.NumFiles; --i >= 0; )
  {
    // Check length
    if (SzArEx_GetFileNameUtf16(&mArchive, i, nullptr) > sizeof(Unicode16) - 2)
    {
      { LOG(LogError) << "Filename " << i << " in archive " << mArchivePath << " is too long: ignored."; }
      continue;
    }
    // Extract & convert to UTF8
    int length = (int)SzArEx_GetFileNameUtf16(&mArchive, i, Unicode16) - 1 /* zero terminal */;
    String fileName;
    for(int c = 0; c < length; c++) fileName.AppendUTF8((String::Unicode)Unicode16[c]);
    mPathList.push_back(Path(fileName));
  }
  return !mPathList.empty();
}

void SevenZipInspector::CloseArchive()
{
  SzArEx_Free(&mArchive, &mMainAlloc);
  File_Close(&mFileInStream.file);
}

