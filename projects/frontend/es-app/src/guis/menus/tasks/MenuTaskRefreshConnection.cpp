//
// Created by bkg2k on 10/11/24.
//

#include <guis/menus/tasks/MenuTaskRefreshConnection.h>
#include <liboping/src/oping.h>
#include <guis/menus/base/ItemText.h>

void MenuTaskRefreshConnection::TaskRun()
{
  double timeout = 0.500; // 500 ms timeout
  pingobj_t* pinger = ping_construct();
  ping_host_add(pinger, "recalbox.com");
  ping_host_add(pinger, "google.com");
  ping_host_add(pinger, "8.8.8.8");
  ping_setopt(pinger, PING_OPT_TIMEOUT, &timeout);

  while(Running())
  {
    for(int i = mLastPingStatus ? 100 : 25; --i >= 0; )
      if (Running()) Thread::Sleep(20);
      else return;
    bool pingOk = ping_send(pinger) > 0;
    mSender.Send(pingOk);
  }
  ping_destroy(pinger);
}

void MenuTaskRefreshConnection::ReceiveSyncMessage(bool connected)
{
  mConnected.SetText(connected ? _("CONNECTED") : _("NOT CONNECTED"));
}
