//
// Created by bkg2k on 05/12/2019.
//

#include "HttpClient.h"
#include "utils/Files.h"
#include "utils/Log.h"

HttpClient::DownloadInfo HttpClient::sDownloadStorage[sMaxBandwidthInfo];
int HttpClient::sDownloadCount = 0;
int HttpClient::sDownloadIndex = 0;
Mutex HttpClient::sDownloadLocker;

HttpClient::HttpClient() noexcept
  : mHandle(curl_easy_init())
  , mStringList(nullptr)
  , mIDownload(nullptr)
  , mContentSize(0)
  , mContentLength(0)
  , mContentFlushed(0)
  , mLastReturnCode(0)
  , mCancel(false)
{
  if (mHandle != nullptr)
  {
    curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
    curl_easy_setopt(mHandle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(mHandle, CURLOPT_TCP_KEEPIDLE, 240L);
    curl_easy_setopt(mHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(mHandle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(mHandle, CURLOPT_XFERINFOFUNCTION, XferInfoCallback);
    curl_easy_setopt(mHandle, CURLOPT_XFERINFODATA, this);
    mCookieFile = Path(String(sCookieFileBase).Append(".curl"));
    curl_easy_setopt(mHandle, CURLOPT_COOKIEFILE, mCookieFile.ToChars());
    curl_easy_setopt(mHandle, CURLOPT_COOKIEJAR, mCookieFile.ToChars());
  }
}

HttpClient::~HttpClient()
{
  if (mStringList != nullptr)
    curl_slist_free_all(mStringList);
  if (mHandle != nullptr)
    curl_easy_cleanup(mHandle);
  (void)mCookieFile.Delete();
}


bool HttpClient::GetFileSize(const String& url, int& size)
{
  size = -1;
  if (mHandle != nullptr)
  {
    curl_easy_setopt(mHandle, CURLOPT_HEADER, 1);
    curl_easy_setopt(mHandle, CURLOPT_NOBODY, 1);
    curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
    CURLcode res = curl_easy_perform(mHandle);
    curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &mLastReturnCode);
    bool ok = (res == CURLcode::CURLE_OK);
    if (!ok) { LOG(LogError) << "[HttpClient] Curl error code: " << res; }
    if (ok)
    {
      curl_off_t cl;
      curl_easy_getinfo(mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
      size = (int)cl;
      return true;
    }
  }
  return false;
}

bool HttpClient::Execute(const String& url, String& output, bool autoResume, int tries)
{
  if (mHandle != nullptr)
  {
    DateTime start;
    mContentSize = 0;
    mContentLength = 0;
    mLastReturnCode = 0;
    mResultHolder.clear();
    mResultFile = Path::Empty;
    bool ok = false;
    CURLcode res = CURLE_OK;
    for(int i = tries > 0 ? tries : 1; --i >= 0;)
    {
      curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
      if (autoResume && mContentSize != 0)
      {
        mContentSize -= 1024;
        mContentSize &= ~1023;
        mResultHolder.Delete(mContentSize, mResultHolder.Count() - mContentSize);
        curl_easy_setopt(mHandle, CURLOPT_RESUME_FROM, (long)mContentSize);
      }
      else
      {
        // Clear any stale resume offset from a previous Execute() call on this handle
        curl_easy_setopt(mHandle, CURLOPT_RESUME_FROM, 0L);
      }
      DataStart();
      res = curl_easy_perform(mHandle);
      curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &mLastReturnCode);
      StoreDownloadInfo(start, DateTime(), mContentSize);
      output = std::move(mResultHolder);
      ok = (res == CURLcode::CURLE_OK);
      // HTTP 416: resume offset is beyond file size. Restart from scratch on the next iteration
      if (ok && mLastReturnCode == 416)
      {
        mContentSize = 0;
        mResultHolder.clear();
        output.clear();
        ok = false;
      }
      if (ok) break;
    }
    if (!ok) { LOG(LogError) << "[HttpClient] Curl error code: " << res; }
    if (ok) DataEnd();
    else
    {
      { LOG(LogWarning) << "[Http] CURL error code: " << res; }
    }
    return ok;
  }
  return false;
}

bool HttpClient::Execute(const String& url, const Path& output, bool autoResume, int tries)
{
  if (mHandle != nullptr)
  {
    if (!output.Directory().Exists())
      (void)output.Directory().CreatePath();
    DateTime start;
    mContentSize = 0;
    mContentLength = 0;
    mContentFlushed = 0;
    mLastReturnCode = 0;
    mResultHolder.clear();
    mResultFile = output;
    (void)mResultFile.Delete();
    bool ok = false;
    CURLcode res = CURLE_OK;
    for(int i = tries > 0 ? tries : 1; --i >= 0;)
    {
      if (autoResume && mContentSize != 0)
      {
        mContentSize -= 1024;
        mContentSize &= ~1023;
        mContentFlushed = mContentSize;
        mResultHolder.clear();
        curl_easy_setopt(mHandle, CURLOPT_RESUME_FROM, (long)mContentSize);
      }
      else
      {
        // Clear any stale resume offset from a previous Execute() call on this handle
        curl_easy_setopt(mHandle, CURLOPT_RESUME_FROM, 0L);
      }
      DataStart();
      curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
      res = curl_easy_perform(mHandle);
      WriteCallback(nullptr, 0, 0, this);
      curl_easy_getinfo(mHandle, CURLINFO_RESPONSE_CODE, &mLastReturnCode);
      StoreDownloadInfo(start, DateTime(), mContentSize);
      ok = (res == CURLcode::CURLE_OK);
      // HTTP 416: resume offset is beyond file size. Restart from scratch on the next iteration
      if (ok && mLastReturnCode == 416)
      {
        mContentSize = 0;
        mContentFlushed = 0;
        mResultHolder.clear();
        (void)mResultFile.Delete();
        ok = false;
      }
      if (ok) break;
    }
    if (!ok)
    {
      { LOG(LogWarning) << "[HttpClient] CURL error code: " << res << ", HTTP code: " << mLastReturnCode; }
      (void)output.Delete();
    }
    else DataEnd();
    return ok;
  }
  return false;
}

bool HttpClient::Execute(const String& url, const Path& output, HttpClient::IDownload* interface, bool autoResume, int tries)
{
  mIDownload = interface;
  return Execute(url, output, autoResume, tries);
}

size_t HttpClient::WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata)
{
  HttpClient& This = *((HttpClient*)userdata);

  return This.DoDataReceived(ptr, (int)(size * nmemb));
}

int HttpClient::XferInfoCallback(void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
  (void)dltotal; (void)dlnow; (void)ultotal; (void)ulnow;
  HttpClient& This = *((HttpClient*)userdata);

  return This.DoXferInfo();
}

int HttpClient::DoXferInfo()
{
  return mCancel ? 1 : 0;
}

size_t HttpClient::DoDataReceived(const char* data, int length)
{
  // Always store into the string
  mResultHolder.Append(data, length);
  mContentSize += (long long)(length);

  // Get content length
  if (mContentLength == 0)
  {
    double contentLength = 0;
    curl_easy_getinfo(mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
    mContentLength = (long long)contentLength;
  }

  // Callback?
  if (mIDownload != nullptr)
    mIDownload->DownloadProgress(*this, mContentSize, mContentLength);

  // Inherited processing
  DataReceived(data, length);

  // Should flush?
  if (!mResultFile.IsEmpty())
    if (mResultHolder.length() > sMaxDataKeptInRam || length == 0)
    {
      // Try flushing to disk
      if (!Files::WriteToFile(mResultFile, mResultHolder.c_str(), (int)mResultHolder.length(), (int)mContentFlushed))
        return 0; // Error flushing to disk
      mContentFlushed += (int)mResultHolder.length();
      // Clear the string
      mResultHolder.clear();
    }

  return mCancel ? 0 : length;
}

void HttpClient::Cancel()
{
  //if (mHandle != nullptr)
  //  curl_easy_reset(mHandle);
  mCancel = true;
}

void HttpClient::SetBasicAuth(const String& login, const String& password)
{
  if (mHandle != nullptr)
  {
    // Hold strings
    mLogin = login;
    mPassword = password;
    // Set basic auth
    curl_easy_setopt(mHandle, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
    curl_easy_setopt(mHandle, CURLOPT_USERNAME, mLogin.data());
    curl_easy_setopt(mHandle, CURLOPT_PASSWORD, mPassword.data());
  }
}

void HttpClient::SetBearer(const String& bearer)
{
  if (mHandle != nullptr)
  {
    // Hold strings
    mBearer = bearer;
    // Set bearer auth
    if (mStringList != nullptr) curl_slist_free_all(mStringList);
    mStringList = nullptr;
    const String header = String("Authorization: Bearer ").Append(bearer);
    mStringList = curl_slist_append(mStringList, header.c_str());
    curl_easy_setopt(mHandle, CURLOPT_HTTPHEADER, mStringList);
  }
}

void HttpClient::CancelBasicAuth()
{
  if (mHandle != nullptr)
  {
    // Reset strings
    mLogin.clear();
    mPassword.clear();
    // Set basic auth
    curl_easy_setopt(mHandle, CURLOPT_HTTPAUTH, (long)CURLAUTH_NONE);
    curl_easy_setopt(mHandle, CURLOPT_USERNAME, mLogin.data());
    curl_easy_setopt(mHandle, CURLOPT_PASSWORD, mPassword.data());
  }
}

void HttpClient::StoreDownloadInfo(const DateTime& start, const DateTime& stop, long long size)
{
  long long elapsed = (stop - start).TotalMilliseconds();
  Mutex::AutoLock locker(sDownloadLocker);
  DownloadInfo& info = sDownloadStorage[sDownloadIndex];
  info.Time = elapsed;
  info.Size = size;
  if (++sDownloadIndex >= sMaxBandwidthInfo) sDownloadIndex = 0;
  sDownloadCount++;

  { LOG(LogDebug) << "[Http] Average Bandwidth: " << (float)GetAverageBandwidth() << " bytes/s."; }
}

double HttpClient::GetAverageBandwidth()
{
  if (sDownloadCount < 16) return -1; // Not enough data
  Mutex::AutoLock locker(sDownloadLocker);
  int count = sDownloadCount > sMaxBandwidthInfo ? sMaxBandwidthInfo : sDownloadCount;
  long long accumulatedBytes = 0;
  long long accumulatedTime = 0;
  for(int i = count; --i >= 0;)
  {
    accumulatedBytes += sDownloadStorage[i].Size;
    accumulatedTime += sDownloadStorage[i].Time;
  }
  return (double)accumulatedBytes / ((double)accumulatedTime / 1000.0);
}
