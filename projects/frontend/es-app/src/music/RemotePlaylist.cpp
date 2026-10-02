//
// Created by bkg2k on 29/05/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "RemotePlaylist.h"
#include "rapidjson/document.h"
#include "recalbox/RecalboxSystem.h"
#include "IRemotePlaylistNotification.h"
#include "network/Networks.h"
#include <patreon/PatronInfo.h>
#include <RecalboxConf.h>
#include <utils/Files.h>
#include <sys/statvfs.h>
#include <algorithm>
#include <random>

#include "utils/Sizes.h"
#include "utils/storage/LeastRecentlyUsed.h"

static String FormatCacheStats(long long int used, long long int limit)
{
  int pctFree = (limit > 0) ? (int)(100LL * (limit - used) / limit) : 0;
  return String(Sizes(used).ToHumanSize()).Append(" used / ").Append(Sizes(limit).ToHumanSize()).Append(" limit (").Append(pctFree).Append("% free)");
}

const String RemotePlaylist::TrackInfo::sNoImage(":/no_image.png");

RemotePlaylist::RemotePlaylist(IRemotePlaylistNotification* callback)
  : StaticLifeCycleControler("RemotePlaylist")
  , mSender(*this)
  , mProgressSender(*this)
  , mStorageRoot(sStorageRootPath)
  , mDownloadInterface(nullptr)
  , mRandomMode(true)
  , mCallback(callback)
{
  LoadDisabledList();
  Thread::Start("RemotePlaylist");
}

RemotePlaylist::~RemotePlaylist()
{
  mRequest.Cancel();
  mAction.Fire();
  Thread::Stop();
}

void RemotePlaylist::Run()
{
  // Wait for Patreon response
  PatronInfo::Instance().WaitForAuthentication(*this);

  // Check patron status
  if (PatronInfo::Instance().Status() == PatronAuthenticationResult::Patron)
  {
    // Main loop
    while(IsRunning())
    {
      // No playlist means the download failed: keep trying, a patron must not be left on the default musics
      if (!HasTracks())
      {
        if (!UpdatePlayList()) { mAction.WaitSignal(sPlaylistRetryDelay); continue; }
        InitializeIndex();
      }
      // Sanitize files
      long long int cachedFileSize = SanitizeFiles();
      // Download new files
      DownloadFiles(cachedFileSize);
      if (!IsRunning()) break;
      // Wait for action
      mAction.WaitSignal();
    }
  }
  else { LOG(LogError) << "[RemotePlaylist] Not a patron"; }
}

bool RemotePlaylist::DeserializePlaylist(const String& jsonString)
{
  // Deserialize
  rapidjson::Document json;
  json.Parse(jsonString.c_str());
  if (!json.HasParseError() && json.IsArray())
  {
    Mutex::AutoLock locker(mLocker);
    mPlaylist.clear();
    for(const rapidjson::Value& object : json.GetArray())
      if (object.HasMember("name") &&
          object.HasMember("time_seconds") &&
          object.HasMember("size") &&
          object.HasMember("url") &&
          //object.HasMember("album_image") &&
          //object.HasMember("image") &&
          object.HasMember("md5") &&
          object.HasMember("album") &&
          object.HasMember("mixtape") &&
          object.HasMember("artist"))
      {
        mPlaylist.push_back(TrackInfo(object["name"].GetString(),
                                      object["time_seconds"].GetInt(),
                                      object["size"].GetInt(),
                                      object["url"].GetString(),
                                      object.HasMember("album_image") ? object["album_image"].GetString() : String::Empty,
                                      object.HasMember("image") ? object["image"].GetString() : String::Empty,
                                      object["md5"].GetString(),
                                      object["album"].GetString(),
                                      object["mixtape"].GetString(),
                                      object["artist"].GetString()));
        mPlaylist.back().SetEnabled(!mDisabledTracks.contains(mPlaylist.back().Md5()));
      }
    mCursor.Reset((int)mPlaylist.size(), mCursor.Index());
    { LOG(LogDebug) << "[RemotePlaylist] Downloaded a playlist with : " << mPlaylist.size() << " musics"; }
    // Cache list
    Files::SaveFile(Path(sStorageList), jsonString);
    return true;
  }
  { LOG(LogError) << "[RemotePlaylist] Error getting playlist. Malformed json: " << jsonString; }
  return false;
}

long long int RemotePlaylist::GetCacheLimit()
{
  long long int cacheLimit = 256LL << 20;
  if (struct statvfs stats {}; statvfs(sStorageRootPath, &stats) == 0)
    cacheLimit = ((long long int)stats.f_bfree * (long long int)stats.f_bsize) - (long long int)(128LL << 20);
#ifdef DEBUG
  cacheLimit = 512LL << 20;
#endif
  cacheLimit = std::max(0LL, cacheLimit);
  { LOG(LogDebug) << "[RemotePlaylist] Cache limit: " << Sizes(cacheLimit).ToHumanSize(); }
  return cacheLimit;
}

bool RemotePlaylist::UpdatePlayList()
{
  String jsonString;

  // Remote
  mRequest.SetBearer(RecalboxConf::Instance().GetRecalboxPrivateKey());
  if (mRequest.Execute(sRemotePlayListUrl, jsonString))
  {
    if (mRequest.GetLastHttpResponseCode() == 200)
      return DeserializePlaylist(jsonString);
    { LOG(LogError) << "[RemotePlaylist] Error getting playlist. Error code: " << mRequest.GetLastHttpResponseCode(); }
  }
  else { LOG(LogError) << "[RemotePlaylist] Error getting playlist."; }

  // Local: an outdated playlist still beats falling back on the theme musics
  jsonString = Files::LoadFile(Path(sStorageList));
  return !jsonString.empty() && DeserializePlaylist(jsonString);
}

void RemotePlaylist::InitializeIndex()
{
  // Initialize main path
  (void)mStorageRoot.CreatePath();

  // Try loading index
  Path indexPath(sStorageIndex);
  String md5Index = Files::LoadFile(indexPath);
  Mutex::AutoLock locker(mLocker);
  mCursor.Reset((int)mPlaylist.size(), LookupTrackIndexLocked(md5Index));
  { LOG(LogDebug) << "[RemotePlaylist] Found index to start from : " << mCursor.Index() << " (" << md5Index << ")"; }
}

void RemotePlaylist::UpdateIndex() const
{
  int index = mCursor.Index();
  if ((unsigned int)index >= (unsigned int)mPlaylist.size()) return;
  Path indexPath(sStorageIndex);
  { LOG(LogDebug) << "[RemotePlaylist] Setting next index file to " << index << "(" << mPlaylist[index].Name() << ")";}
  Files::SaveFile(indexPath, mPlaylist[index].Md5());
}

int RemotePlaylist::LookupTrackIndexLocked(const String& md5) const
{
  for(int i = (int)mPlaylist.size(); --i >= 0; )
    if (md5 == mPlaylist[i].Md5())
      return i;
  return -1;
}

int RemotePlaylist::LookupTrackIndex(const String& md5) const
{
  Mutex::AutoLock locker(mLocker);
  return LookupTrackIndexLocked(md5);
}

long long int RemotePlaylist::SanitizeFiles()
{
  if (mPlaylist.empty()) return 0;

  // Remove unknown files and accumulate size of known files only
  long long int totalSize = 0;
  for(const Path& file : Path(sStorageRootPath).GetDirectoryContent())
  {
    if (LookupTrackIndex(file.FilenameWithoutExtension()) < 0)
    {
      (void)file.Delete();
      { LOG(LogDebug) << "[RemotePlaylist] File " << file.ToString() << " not in playlist. Deleted."; }
    }
    else
      totalSize += file.Size();
  }

  long long int cacheLimit = GetCacheLimit();
  { LOG(LogDebug) << "[RemotePlaylist] After sanitize - " << FormatCacheStats(totalSize, cacheLimit); }

  (void)EvictDownTo(cacheLimit, totalSize);

  return totalSize;
}

bool RemotePlaylist::EvictDownTo(long long int budget, long long int& totalSize)
{
  if (totalSize <= budget) return true;

  // One listing, one stat per file: the election below runs on memory only
  Path::PathList files(Path(sStorageRootPath).GetDirectoryContent());
  std::vector<long long int> accessTimes;
  accessTimes.reserve(files.size());
  for(const Path& file : files) accessTimes.push_back(file.AccessDateTime().ToEpochTime());

  while (totalSize > budget)
  {
    LeastRecentlyUsed oldest;
    for (int i = (int)files.size(); --i >= 0; )
      if (!files[i].IsEmpty()) oldest.Consider(i, accessTimes[i]);
    if (!oldest.Found()) return false;

    long long int deletedSize = files[oldest.Index()].Size();
    { LOG(LogDebug) << "[RemotePlaylist] Cache full, deleting oldest: " << files[oldest.Index()].ToString() << " (" << Sizes(deletedSize).ToHumanSize() << ") - " << FormatCacheStats(totalSize, budget); }
    (void)files[oldest.Index()].Delete();
    files[oldest.Index()] = Path::Empty;
    totalSize = std::max(0LL, totalSize - deletedSize);
  }
  return true;
}

void RemotePlaylist::DownloadFiles(long long int totalSize)
{
  if (mPlaylist.empty()) return;

  mLocker.Lock();
  int index = mCursor.Index();
  mLocker.UnLock();
  int orgIndex = index;
  int cachedAhead = 0;

  long long int cacheLimit = GetCacheLimit();
  { LOG(LogDebug) << "[RemotePlaylist] Starting download - " << FormatCacheStats(totalSize, cacheLimit); }

  while(IsRunning())
  {
    // Capture playlist state and track info under lock to avoid races with Randomize()
    int newIndex = -1;
    bool songRequested;
    bool skipped;
    TrackInfo track;
    {
      Mutex::AutoLock locker(mLocker);
      songRequested = !mSongRequest.empty();
      if (songRequested)
      {
        newIndex = LookupTrackIndexLocked(mSongRequest);
        if (newIndex < 0)
        {
          { LOG(LogError) << "[RemotePlaylist] Requested song not found ! " << mSongRequest; }
          songRequested = false;
        }
      }
      int actualIndex = (newIndex >= 0) ? newIndex : index;
      if (actualIndex >= (int)mPlaylist.size()) break;
      track = mPlaylist[actualIndex];
      // A track the listener turned off will never be played: the cache must not be spent on it
      skipped = !songRequested && !track.IsEnabled();
    }
    if (skipped)
    {
      index = (index + 1) % (int)mPlaylist.size();
      if (index == orgIndex) break;
      continue;
    }
    // If cache is full and no song is explicitly requested, free space by deleting oldest files
    if (!songRequested && totalSize + track.Size() > cacheLimit)
      if (!EvictDownTo(cacheLimit - track.Size(), totalSize)) break;
    // A file whose size does not match is a truncated transfer, and it is never repaired otherwise
    if (!track.HasValidFile())
    {
      long long int alreadyOnDisk = track.LocalPath().Size(); // dropped by Execute, stops counting
      // Download
      { LOG(LogDebug) << "[RemotePlaylist] Downloading " << track.LocalPath().ToString() << " at index " << index << " - " << FormatCacheStats(totalSize, cacheLimit); }
      ReportDownload(track, DownloadState::Start, false);
      if (!mRequest.Execute(track.Url(), track.LocalPath(), this, true, 3))
      {
        // Download failed
        if (!IsRunning()) return;
        { LOG(LogError) << "[RemotePlaylist] Failed to download " << track.Url() << ". Retry in a few seconds."; }
        ReportDownload(track, DownloadState::End, false);
        Thread::Sleep(5000);
        break;
      }
      if (!IsRunning()) return;
      // Invalid HTTP return code ?
      if (mRequest.GetLastHttpResponseCode() / 100 != 2)
      {
        if (!IsRunning()) return;
        { LOG(LogError) << "[RemotePlaylist] Failed to download " << track.Url() << ". Http Result code: " << mRequest.GetLastHttpResponseCode() << ". Retry in a few seconds."; }
        ReportDownload(track, DownloadState::End, false);
        Thread::Sleep(2000);
        break;
      }
      // Invalid hash ?
      /*if (mRequest.Md5().ToLowerCase() != track.Md5().ToLowerCase())
      {
        if (!IsRunning()) return;
        { LOG(LogError) << "[RemotePlaylist] Failed to download " << track.Url() << ". MD5 mismatch. Retry."; }
        ReportDownload(track, DownloadState::End, false);
        continue;
      }*/
      // Good, download is ok
      if (!IsRunning()) return;
      long long int actualSize = track.LocalPath().Size();
      { LOG(LogDebug) << "[RemotePlaylist] Downloaded " << track.LocalPath().ToString() << " (" << Sizes(actualSize).ToHumanSize() << ") - " << FormatCacheStats(totalSize, cacheLimit); }
      ReportDownload(track, DownloadState::End, true);
      mSender.Send(); // Playlist is ready notification
      totalSize += actualSize - alreadyOnDisk;
    }
    // Notify the track is available for playing
    else ReportDownload(track, DownloadState::End, true);

    if (songRequested)
    {
      // Safety: clear the request even if ReportDownload skipped it (mDownloadInterface was null),
      // so the next iteration switches to normal lookahead mode.
      // Only clear if the request still matches the track we just handled.
      Mutex::AutoLock locker(mLocker);
      if (mSongRequest == track.Md5())
        mSongRequest.clear();
      // Do NOT break here — continue downloading the lookahead immediately after
      // the requested song, so tracks are ready when it finishes playing.
    }
    else
    {
      // Normal lookahead: advance index and check termination conditions
      cachedAhead++;
      index = (index + 1) % (int)mPlaylist.size();
      if (index == orgIndex) break;
      if (cachedAhead >= sLookAheadCount) break;
    }
  }
}

void RemotePlaylist::ReportDownload(const TrackInfo& track, DownloadState state, bool finalStatus)
{
  Mutex::AutoLock locker(mLocker);
  if (mDownloadInterface == nullptr) return;
  if (track.Md5() != mSongRequest) return;
  switch (state)
  {
    case DownloadState::Start:
      mDownloadInterface->StartDownloadingSong(mSongRequest);
      break;
    case DownloadState::End:
      mDownloadInterface->EndDownloadingSong(mSongRequest, finalStatus);
      mSongRequest.clear();
      break;
  }
}

RemotePlaylist::TrackInfo* RemotePlaylist::GetNextTrack()
{
  if (!RecalboxConf::Instance().GetMusicRemoteEnable()) return nullptr;

  {
    Mutex::AutoLock locker(mLocker);
    // Scan the whole playlist: any downloaded track is a better answer than the default musics
    int index = mCursor.Seek([this](int i) { return mPlaylist[i].IsEnabled() && mPlaylist[i].HasValidFile(); });
    if (index != PlaylistCursor::sNoTrack)
    {
      { LOG(LogDebug) << "[RemotePlaylist] Next track peek at index " << index << " : " << mPlaylist[index].Name() << " (" << mPlaylist[index].Md5() << ")"; }
      return &mPlaylist[index]; // Pure peek: the head moves past it only when TrackAccepted() is called
    }
  }
  // No file found, let's go download - the head stays where it is so the download starts there
  { LOG(LogDebug) << "[RemotePlaylist] No next track found, will download"; }
  mAction.Fire();
  return nullptr;
}

void RemotePlaylist::TrackAccepted()
{
  Mutex::AutoLock locker(mLocker);
  if (!mPlaylist.empty())
  {
    mCursor.Advance();
    UpdateIndex();
    mAction.Fire();
  }
}

void RemotePlaylist::TrackConsumed()
{
  mAction.Fire();
}

void RemotePlaylist::RequestSong(const String& songMD5)
{
  { Mutex::AutoLock lock(mLocker); mSongRequest = songMD5; }
  mRequest.Cancel();
  mAction.Fire();
}

void RemotePlaylist::ReceiveSyncMessage()
{
  if (mCallback != nullptr)
    mCallback->PlaylistReady();
}

void RemotePlaylist::ReceiveSyncMessage(const RemoteTrackDownloadProgress& message)
{
  if (mDownloadInterface != nullptr)
    if (IsSongRequested())
      mDownloadInterface->DownloadingSong(GetRequestedSong(), message.mCurrentSize, message.mTotalSize);
}

void RemotePlaylist::DownloadProgress(const HttpClient& http, long long currentSize, long long expectedSize)
{
  (void)http;
  mProgressSender.Send({ .mCurrentSize=currentSize, .mTotalSize=expectedSize });
}

void RemotePlaylist::LoadDisabledList()
{
  String content = Files::LoadFile(Path(sDisabledListPath));
  if (!content.empty())
    for (const String& md5 : content.Split('\n'))
      if (!md5.empty())
        mDisabledTracks.insert(md5);
  { LOG(LogDebug) << "[RemotePlaylist] Loaded " << mDisabledTracks.size() << " disabled tracks"; }
}

void RemotePlaylist::SaveDisabledList() const
{
  String content;
  for (const String& md5 : mDisabledTracks)
  {
    if (!content.empty()) content.Append('\n');
    content.Append(md5);
  }
  Files::SaveFile(Path(sDisabledListPath), content);
}

void RemotePlaylist::SetTrackEnabled(const String& md5, bool enabled)
{
  Mutex::AutoLock locker(mLocker);
  if (enabled)
    mDisabledTracks.erase(md5);
  else
    mDisabledTracks.insert(md5);
  // Update the playlist entry
  for (TrackInfo& track : mPlaylist)
    if (track.Md5() == md5)
    { track.SetEnabled(enabled); break; }
  SaveDisabledList();
}

void RemotePlaylist::Randomize(bool randomOn)
{
  Mutex::AutoLock locker(mLocker);
  mRandomMode = randomOn;

  // Remember the current track MD5 to reposition the head after reordering
  String currentMd5;
  if (int index = mCursor.Index(); (unsigned int)index < (unsigned int)mPlaylist.size())
    currentMd5 = mPlaylist[index].Md5();

  if (randomOn)
  {
    // Shuffle the playlist
    std::random_device rd;
    std::mt19937 rng(rd());
    std::shuffle(mPlaylist.begin(), mPlaylist.end(), rng);
  }
  else
  {
    // Sort by mixtape then by name (same order as the Jukebox display)
    std::sort(mPlaylist.begin(), mPlaylist.end(), [](const TrackInfo& a, const TrackInfo& b) { return a.Compare(b); });
  }

  // Reposition the head on the same track (mLocker already held)
  mCursor.Reset((int)mPlaylist.size(), currentMd5.empty() ? 0 : LookupTrackIndexLocked(currentMd5));
}

