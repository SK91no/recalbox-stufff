//
// Created by bkg2k on 10/03/24.
//

#include <utils/SecuredFile.h>
#include <utils/Files.h>
#include <utils/Log.h>
#include <unistd.h>

bool SecuredFile::SaveSecuredFile(const Path& path, const String& content, const String& logname, bool autoBackup, IValidationInterface* validator)
{
  // Save to tmp file
  Path tmp = path.ChangeExtension(sTemporaryExtension);
  if (Files::SaveFile(tmp, content))
  {
    // Force sync to disk
    sync();
    // Check size
    if (tmp.Size() == content.Count())
    {
      if (validator == nullptr || validator->ValidateContent(Files::LoadFile(tmp)))
      {
        // Delete old list or rename tpo backup
        if (autoBackup)
        {
          Path backup = path.ChangeExtension(".backup");
          (void)backup.Delete();
          if (!Path::Rename(path, backup))
            { LOG(LogInfo) << "[SecuredFile] Failed to backup file " << path << " to " << backup << " (" << logname << ") of size: " << path.Size(); }
        }
        else (void)path.Delete();
        // Rename tmp to target
        if (Path::Rename(tmp, path))
        {
          // Sync again
          sync();
          { LOG(LogInfo) << "[SecuredFile] Saved file " << path << " (" << logname << ") of size: " << path.Size(); }
          return true;
        }
        LOG(LogError) << "[SecuredFile] Failed to rename " << tmp << " into " << path << " (" << logname << ')';
      }
      else { LOG(LogError) << "[SecuredFile] Invalid content in saved file " << tmp << " (" << logname << ')'; }
    }
    else { LOG(LogError) << "[SecuredFile] Invalid size of saved file " << tmp << " (" << logname << ')'; }
  }
  else { LOG(LogError) << "[SecuredFile] Failed to save " << tmp << " (" << logname << ')'; }
  return false;
}

bool SecuredFile::LoadSecuredFile(bool mandatory, const Path& path, const Path& fallback, String& content, const String& logname, bool autoBackup, IValidationInterface* validator)
{
  Path tmp = path.ChangeExtension(sTemporaryExtension);

  // Try loading from regular file
  if (Load(path, logname, content, validator))
  {
    if (tmp.Exists()) { LOG(LogWarning) << "[SecuredFile] Temporary file found. Possible data loss. " << tmp << " (" << logname << ')'; }
    { LOG(LogDebug) << "[SecuredFile] Loaded content from file " << path << " (" << logname << ')'; }
    return true;
  }

  // Try loading a previously forgotten tmp file
  if (Load(tmp, logname, content, validator))
  {
    { LOG(LogDebug) << "[SecuredFile] Loaded content from forgotten temp file " << tmp << " instead of " << path << " (" << logname << ')'; }
    return true;
  }

  // Try backup if any
  if (autoBackup)
  {
    Path backup = path.ChangeExtension(".backup");
    if (Load(backup, logname, content, validator))
    {
      { LOG(LogDebug) << "[SecuredFile] Loaded content from backup file " << tmp << " instead of " << path << " (" << logname << ')'; }
      return true;
    }
  }

  // Try loading from fallback
  if (!fallback.IsEmpty())
  {
    if (!fallback.Exists()) return true; // No backup exists, continue
    if (Load(fallback, logname, content, validator))
    {
      { LOG(LogDebug) << "[SecuredFile] Loaded content from fallback file " << fallback << " instead of " << path << " (" << logname << ')'; }
      return true;
    }
  }

  if (mandatory)
    { LOG(LogError) << "[SecuredFile] No file has been loaded for file " << path << " (" << logname << ')'; }
  return false;
}

bool SecuredFile::Load(const Path& path, const String& logname, [[out]] String& content, IValidationInterface* validator)
{
  if (!path.Exists()) return false;

  if (path.Size() != 0)
  {
    content = Files::LoadFile(path);
    if (path.Size() == content.Count())
    {
      if (validator == nullptr || validator->ValidateContent(content)) return true;
      { LOG(LogError) << "[SecuredFile] Invalid content in load file " << path << " (" << logname << ')'; }
    }
    else { LOG(LogError) << "[SecuredFile] Invalid size of loaded file " << path << " (" << logname << ')'; }
  }
  else { LOG(LogError) << "[SecuredFile] Invalid size of not yet loaded file " << path << " (" << logname << ')'; }

  return false;
}
