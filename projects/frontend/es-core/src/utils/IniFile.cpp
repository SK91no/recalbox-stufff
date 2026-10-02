//
// Created by thierry.imbert on 18/02/2020.
// Last modification by Maksthorr on 28/04/2023
//

#include "IniFile.h"

#include <utils/Files.h>
#include "utils/Log.h"

String IniFile::sAllowedCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-");

#ifdef DEBUG
void ReroutePath(Path& path)
{
  if (path.StartWidth("/boot/"))
  {
    Path reRoutedBoot("/recalbox/share/system/boot/");
    (void)reRoutedBoot.CreatePath();
    bool dummy;
    path = reRoutedBoot / path.MakeRelative(Path("/boot/"), dummy);
    (void)path.Directory().CreatePath();
  }
}
#endif

IniFile::IniFile(const Path& path, const Path& fallbackpath, const String& logname, bool extraSpace, bool autoBackup, bool allowDuplicates)
  : mLocker(new Mutex())
  , mLogName(logname)
  , mFilePath(path)
  , mFallbackFilePath(fallbackpath)
  , mExtraSpace(extraSpace)
  , mAutoBackup(autoBackup)
  , mAllowDuplicates(allowDuplicates)
  , mValid(false)
{
  #ifdef DEBUG
  ReroutePath(mFilePath);
  ReroutePath(mFallbackFilePath);
  #endif
  mValid = Load();
}

IniFile::IniFile(const Path& path, const String& logname, bool extraSpace, bool autoBackup, bool allowDuplicates)
  : mLocker(new Mutex())
  , mLogName(logname)
  , mFilePath(path)
  , mExtraSpace(extraSpace)
  , mAutoBackup(autoBackup)
  , mAllowDuplicates(allowDuplicates)
  , mValid(false)
{
  #ifdef DEBUG
  ReroutePath(mFilePath);
  #endif
  mValid = Load();
}

void IniFile::PurgeKey(String& key)
{
  for(char& c : key)
    if (sAllowedCharacters.Find(c) < 0)
      c = '-';
}

bool IniFile::ValidateContent(const String& content)
{
  // An empty file may have been a wrecked file. Invalidate.
  if (content.Count() == 0) return false;

  int faults = 0;
  String key;
  String value;
  for(String& line : content.Split('\n'))
  {
    line.Trim();
    if (line.empty()) continue; // Empty line ok
    if (line[0] == '#') continue; // Comment line
    if (line.Extract('=', key, value, true))
      if (!key.empty())
      {
        if (key[0] == ';') key.erase(0, 1);
        if (!key.empty())
          if (key.find_first_not_of(sAllowedCharacters) == String::npos) continue; // ok !
      }
    { LOG(LogDebug) << "[IniFile] Line not validated: " << line; }
    faults++;
  }

  return faults <= 3;
}

bool IniFile::IsValidKeyValue(const String& line, String& key, String& value, bool& isCommented)
{
  if (!line.empty()) // Ignore empty line
  {
    bool comment = (line[0] == '#');
    if (!comment)
    {
      int separatorPos = line.Find('=');
      if (separatorPos >= 0) // Expect a key=value line
      {
        key = line.SubString(0, separatorPos).Trim();
        if (isCommented = (!key.empty() && key[0] == ';'); isCommented) key.erase(0, 1);
        value = line.SubString(separatorPos + 1).Trim();
        if (key.find_first_not_of(sAllowedCharacters) == String::npos) return true;
        { LOG(LogWarning) << "[IniFile] Invalid key: `" << key << '`'; }
      }
      else { LOG(LogError) << "[IniFile] Invalid line: `" << line << '`'; }
    }
  }
  return false;
}

bool IniFile::LoadContent(String& content)
{
  bool ok = SecuredFile::LoadSecuredFile(true, mFilePath, mFallbackFilePath, content, mLogName, true, this);
  if (!ok) { LOG(LogError) << "[IniFile] Cannot load " << mFilePath; }
  return ok;
}

bool IniFile::Load()
{
  Mutex::AutoLock locker(*mLocker);

  // Special case of empty path
  if (mFilePath.IsEmpty() && mFallbackFilePath.IsEmpty()) return true; // New or empty file is valid

  // Load file
  String content;
  if (!LoadContent(content)) return false;

  // Split lines
  content.Remove('\r');
  String::List lines = content.Split('\n');
  { LOG(LogDebug) << "[IniFile] Load: " << lines.size() << " lines loaded."; }

  // Get key/value
  String key;
  String value;
  bool comment = false;
  for (String& line : lines)
    if (IsValidKeyValue(line.Trim(), key, value, comment))
      if (!comment)
        mConfiguration[key] = value;

  OnLoad();
  return !mConfiguration.empty();
}

bool IniFile::MakeBootReadOnly()
{
  #ifdef DEBUG
  return true;
  #else
  return system("mount -o remount,ro /boot") == 0;
  #endif
}

bool IniFile::MakeBootReadWrite()
{
  #ifdef DEBUG
  return true;
  #else
  return system("mount -o remount,rw /boot") == 0;
  #endif
}

bool IniFile::Save()
{
  Mutex::AutoLock locker(*mLocker);

  // No change?
  if (mPendingWrites.empty() && mPendingDelete.empty()) return true;

  // Load file
  String content;
  if (!LoadContent(content))
  {
    { LOG(LogError) << "[IniFile] Save: Error loading base failed. Save aborted."; }
    return false;
  }

  // Split lines
  content.Remove('\r');
  String::List lines = content.Split('\n');
  { LOG(LogDebug) << "[IniFile] Save: " << lines.size() << " lines loaded."; }

  // Run throiugh lines
  int replacedLines = 0;
  int addedLines = 0;
  int deletedLines = 0;
  String lineKey;
  String lineVal;
  String equal(mExtraSpace ? " = " : "=");
  HashSet<String> encounteredKeys;

  for (auto& line : lines)
  {
    bool commented = false;
    if (IsValidKeyValue(line.Trim(), lineKey, lineVal, commented))
    {
      // Key alreadu encountered
      if (encounteredKeys.contains(lineKey))
      {
        if (!mAllowDuplicates)
          if (!line.StartsWith(';'))
            line.Insert(0, ';'); // Comment the line
        continue;
      }
      // Try in pending writes
      if (String* value = mPendingWrites.try_get(lineKey); value != nullptr)
      {
        line = String(lineKey).Append(equal).Append(*value);
        mConfiguration[lineKey] = *value;
        mPendingWrites.erase(lineKey);
        replacedLines++;
      }
      // Try pending deletes (no else, cause a pending write may have been deleted also)
      if (mPendingDelete.contains(lineKey))
      {
        if (!line.StartsWith(';')) line.Insert(0, ';');
        mPendingDelete.erase(lineKey);
        deletedLines++;
      }
      // Line encountered
      encounteredKeys.insert(lineKey);
    }
  }

  // Add remaining lines
  addedLines += (int)mPendingWrites.size();
  for(const auto& keyValue : mPendingWrites)
  {
    lines.push_back(String(keyValue.first).Append(equal).Append(keyValue.second));
    mConfiguration[keyValue.first] = keyValue.second;
  }

  // Clear pendings
  mPendingDelete.clear();
  mPendingWrites.clear();

  { LOG(LogDebug) << "[IniFile] Save: " << replacedLines << " values replaced. " << addedLines << " keys/values added. " << deletedLines << " keys commented."; }

  // Save new
  bool result = true;
  bool boot = mFilePath.StartWidth("/boot/");
  if (boot && !MakeBootReadWrite()) { LOG(LogError) <<"[IniFile] Error remounting boot partition (RW)"; }
  if (!SecuredFile::SaveSecuredFile(mFilePath, String::Join(lines, '\n'), mLogName, true, this))
  { LOG(LogError) << "[IniFile] Unable to save " << mFilePath; }
  if (boot && !MakeBootReadOnly()) { LOG(LogError) << "[IniFile] Error remounting boot partition (RO)"; }

  OnSave();
  return result;
}

String IniFile::AsString(const String& name) const
{
  return ExtractValue(name);
}

String IniFile::AsString(const String& name, const String& defaultValue) const
{
  String item = ExtractValue(name);
  if (!item.empty()) return item;
  return defaultValue;
}

bool IniFile::AsBool(const String& name, bool defaultValue) const
{
  String item = ExtractValue(name);
  return (!item.empty()) ? (item.size() == 1 && item[0] == '1') : defaultValue;
}

unsigned int IniFile::AsUInt(const String& name, unsigned int defaultValue) const
{
  String item = ExtractValue(name);
  if (!item.empty())
  {
    long long int value = 0;
    if (item.TryAsInt64(value))
      return (unsigned int)value;
  }

  return defaultValue;
}

int IniFile::AsInt(const String& name, int defaultValue) const
{
  String item = ExtractValue(name);
  if (!item.empty())
  {
    int value = 0;
    if (item.TryAsInt(value))
      return value;
  }

  return defaultValue;
}

void IniFile::Delete(const String& name)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.insert(name);
}

IniFile& IniFile::SetString(const String& name, const String& value)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.erase(name);
  mPendingWrites[name] = value;
  return *this;
}

IniFile& IniFile::SetBool(const String& name, bool value)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.erase(name);
  mPendingWrites[name] = value ? "1" : "0";
  return *this;
}

IniFile& IniFile::SetUInt(const String& name, unsigned int value)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.erase(name);
  mPendingWrites[name] = String((long long)value);
  return *this;
}

IniFile& IniFile::SetInt(const String& name, int value)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.erase(name);
  mPendingWrites[name] = String(value);
  return *this;
}

IniFile& IniFile::SetList(const String& name, const String::List& values)
{
  Mutex::AutoLock locker(*mLocker);

  mPendingDelete.erase(name);
  mPendingWrites[name] = String::Join(values, ',');
  return *this;
}

bool IniFile::isInList(const String& name, const String& value) const
{
  Mutex::AutoLock locker(*mLocker);

  if (!value.empty())
    if (mConfiguration.contains(name) || mPendingWrites.contains(name))
    {
      String s = AsString(name);
      for (int p = s.Find(value); p >= 0; p = s.Find(value, p + value.Count()))
        if (p == 0 || s[p - 1] == ',')
          if (p + value.Count() >= s.Count() || s[p + value.Count()] ==  ',')
            return true;
    }
  return false;
}

const String& IniFile::ExtractValue(const String& key) const
{
  Mutex::AutoLock locker(*mLocker);

  String* item = mPendingWrites.try_get(key);
  if (item == nullptr) item = mConfiguration.try_get(key);
  return (item != nullptr) ? *item : String::Empty;
}

bool IniFile::HasKeyStartingWith(const String& startWidth) const
{
  Mutex::AutoLock locker(*mLocker);

  for (const auto& it : mPendingWrites)
    if (it.first.StartsWith(startWidth))
      return true;

  for (const auto& it : mConfiguration)
    if (it.first.StartsWith(startWidth))
      return true;

  return false;
}

bool IniFile::HasKey(const String& key) const
{
  Mutex::AutoLock locker(*mLocker);

  for (const auto& it : mPendingWrites)
    if (it.first == key)
      return true;

  for (const auto& it : mConfiguration)
    if (it.first == key)
      return true;

  return false;
}

String::List IniFile::GetKeyEndingWith(const String& endWidth) const
{
  Mutex::AutoLock locker(*mLocker);

  String::List result;
  for (const auto& it : mPendingWrites)
    if (it.first.EndsWith(endWidth))
      result.push_back(it.first);

  for (const auto& it : mConfiguration)
    if (it.first.EndsWith(endWidth))
      result.push_back(it.first);

  return result;
}

bool IniFile::ResetWithFallback()
{
  Mutex::AutoLock locker(*mLocker);

  if (!mFallbackFilePath.IsEmpty() && mFallbackFilePath.Exists())
  {
    if(!Files::CopyFile(mFallbackFilePath, mFilePath))
      return false;
  }
  this->Cancel();
  mConfiguration.clear();
  return this->Load();
}

