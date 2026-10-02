//
// Created by bkg2k on 03/11/2020.
//

#include "ProcessTree.h"
#include <unistd.h>
#include <libproc2/pids.h>
#include <cstring>
#include <wait.h>
#include <utils/Log.h>
#include <utils/datetime/DateTime.h>

enum pids_item Items[] = {
    PIDS_ID_PID,
    PIDS_ID_PPID,
    PIDS_ID_TGID,
};
#define ITEMS_COUNT (sizeof Items / sizeof *Items)

enum rel_items
{
  EU_PID, EU_PPID, EU_TGID
};

#define PIDS_GETINT(e) PIDS_VAL(EU_ ## e, s_int, stack)

Array<int> ProcessTree::GetTree(int pid)
{
  int mypid = pid < 0 ? getpid() : pid;

  Array<int> result;

  struct pids_info* info = NULL;
  struct pids_stack* stack;

  if (procps_pids_new(&info, Items, ITEMS_COUNT) < 0)
    return result;

  while ((stack = procps_pids_get(info, PIDS_FETCH_THREADS_TOO)))
  {
    int ppid = PIDS_VAL(EU_PPID, s_int, stack, info);
    int tgid = PIDS_VAL(EU_TGID, s_int, stack, info);
    if (((ppid == mypid) || result.Contains(ppid)) && !result.Contains(tgid))
    {
      { LOG(LogDebug) << "[ProcessTree] Will terminate " << tgid; }
      result.Add(tgid);
    }
  }
  procps_pids_unref(&info);

  result.Sort([](const int& a, const int& b) -> int { return b-a; });
  return result;
}

void ProcessTree::TerminateTree(Array<int>& processes, int millisecondTimeout)
{
  for (int i = processes.Count(); --i >= 0;)
    kill(processes[i], SIGINT);

  if (!WaitTreeDeath(processes, millisecondTimeout))
  {
    { LOG(LogInfo) << "[ProcessTree] Timeout while waiting childs pid, killing with SIGTERM "; }
    KillTree(processes);
  }
}

void ProcessTree::KillTree(const Array<int>& processes)
{
  for (int i = processes.Count(); --i >= 0;)
    kill(processes[i], SIGKILL);
}

bool ProcessTree::WaitTreeDeath(Array<int>& pids, int millisecondTimeout)
{
  DateTime reference;

  int stat = 0;
  while ((DateTime() - reference).TotalMilliseconds() < millisecondTimeout)
  {
    for (int i = pids.Count(); --i >= 0;)
      if (waitpid(pids[i], &stat, WNOHANG) == -1 && errno == ECHILD)
      {
        { LOG(LogTrace) << "[ProcessTree] " << pids[i] << " terminated"; }
        pids.Delete(i);
      }
    if (pids.Empty())
      break;
    usleep(1000);
  }

  return pids.Empty();
}
