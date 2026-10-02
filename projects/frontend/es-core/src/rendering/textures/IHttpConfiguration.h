//
// Created by bkg2k on 15/11/25.
//
#pragma once

class HttpClient;

class IHttpConfiguration
{
  public:
    //! Default constructor
    virtual ~IHttpConfiguration() = default;

    //! Http client provider
    virtual void ConfigureHttpClient(const String& url, HttpClient& client) = 0;
};