//
// Created by bkg2k on 13/07/23.
//

#include "RestApiServer.h"
#include <csignal>
#include "Parameters.h"
#include "server/handlers/RequestHandler.h"
#include "server/Server.h"
#include <utils/Log.h>
#include "RecalboxConf.h"

RestApiServer::RestApiServer(SystemManager& systemManager)
  : mWebManagerSync(systemManager)
  , mRequestHandler(mParam.WWWRoot(), mParam.DefaultFile(), systemManager)
  , mServer(mParam, &mRequestHandler)
{
  // Self-check: the self-registering dispatch must be populated at static-init (before the server
  // serves). A zero here would mean the registrars were not initialized in time (toolchain deferral).
  { LOG(LogInfo) << "[WebManagerSync] Configuration dispatch registry: "
                 << (int)RecalboxConf::GlobalDispatchRegistry().size() << " global / "
                 << (int)RecalboxConf::SystemDispatchRegistry().size() << " system self-registered keys"; }
  Start("rest-api");
}

void RestApiServer::Run()
{
  LOG(LogInfo) << "[RestAPIServer] Recalbox WebApi Server 1.0";

  // Run!
  while(IsRunning())
    try
    {
      mServer.Serve();
      break;
    }
    catch(std::exception& ex)
    {
      LOG(LogError) << "[RestAPIServer] Error running server! Retrying in 5s... (Exception: " << ex.what() << ')';
      Thread::Sleep(5000);
    }

  mServer.Shutdown();
}

void RestApiServer::Break()
{
  mServer.Shutdown();
}
