//
// Created by bkg2k on 13/07/23.
//
#pragma once

#include "utils/os/system/Thread.h"
#include "web/server/Server.h"
#include "web/server/handlers/RequestHandler.h"
#include "web/WebManagerSync.h"

class RestApiServer : public Thread
{
  public:
    //! Constructor
    explicit RestApiServer(SystemManager& systemManager);

    //! Destructor
    ~RestApiServer() override { Stop(); }

  private:
    //! Server parameters
    Parameters mParam;
    //! Web manager <-> main thread configuration bridge (must outlive nothing but be ready before serving)
    WebManagerSync mWebManagerSync;
    //! API Request handler
    RequestHandler mRequestHandler;
    //! Http server
    Server mServer;

    /*
     * Thread implementation
     */

    //! Main thread running function
    void Break() override;

    //! Stop the thread
    void Run() override;
};
