//
// Created for feat/webmanager-sync
//

#include "WebManagerSync.h"
#include <RecalboxConf.h>
#include <systems/SystemManager.h>
#include <systems/SystemData.h>
#include <utils/Log.h>

WebManagerSync::WebManagerSync(SystemManager& systemManager)
  : StaticLifeCycleControler<WebManagerSync>("WebManagerSync")
  , mSystemManager(systemManager)
  , mSender(*this)
{
}

void WebManagerSync::PushConfigChange(const String& fullKey, const String& value)
{
  {
    Mutex::AutoLock locker(mLocker);
    mPending.push_back({ .Key = fullKey, .Value = value, .System = String(), .Remove = false });
  }
  mSender.Send();
}

void WebManagerSync::PushSystemConfigChange(const String& systemName, const String& shortKey, const String& value)
{
  {
    Mutex::AutoLock locker(mLocker);
    mPending.push_back({ .Key = shortKey, .Value = value, .System = systemName, .Remove = false });
  }
  mSender.Send();
}

void WebManagerSync::PushConfigDelete(const String& fullKey)
{
  {
    Mutex::AutoLock locker(mLocker);
    mPending.push_back({ .Key = fullKey, .Value = String(), .System = String(), .Remove = true });
  }
  mSender.Send();
}

void WebManagerSync::ReceiveSyncMessage()
{
  // Drain the queue under lock so the web thread can keep enqueuing while we apply
  std::vector<Change> batch;
  {
    Mutex::AutoLock locker(mLocker);
    if (mPending.empty()) return;
    batch.swap(mPending);
  }

  // Apply on the main thread, exclusively through the typed setters so that watchers fire
  RecalboxConf& conf = RecalboxConf::Instance();
  bool changed = false;
  for(const Change& change : batch)
  {
    if (change.Remove)
    {
      // TODO(feat/webmanager-sync): generic delete does not fire typed watchers yet;
      // unify with the X-macro dispatch (typed Delete##name / DeleteSystem##name).
      conf.Delete(change.Key);
      changed = true;
    }
    else if (change.System.empty())
    {
      if (conf.SetByKey(change.Key, change.Value)) changed = true;
      else { LOG(LogWarning) << "[WebManagerSync] Unhandled configuration key: " << change.Key; }
    }
    else
    {
      SystemData* system = mSystemManager.SystemByName(change.System);
      if (system == nullptr) { LOG(LogError) << "[WebManagerSync] Unknown system: " << change.System; continue; }
      if (conf.SetSystemByKey(*system, change.Key, change.Value)) changed = true;
      else { LOG(LogWarning) << "[WebManagerSync] Unhandled system key: " << change.System << '.' << change.Key; }
    }
  }

  // Persist once for the whole batch
  if (changed) conf.Save();
}
