//
// Created by bkg2k on 03/04/2020.
//
#pragma once

#include <utils/json/JSONBuilder.h>

class RequestHandlerTools
{
  private:
    /*!
     * @brief Get supported system list
     * @return Systems' short names list
     */
    static String GetCommandOutput(const String& command);

    static void SetHeaders(Pistache::Http::ResponseWriter& response);

    //! Flag path
    static Path sFlagPath;

    //! Static map of file being saved before a reset factory for reinstallation
    static HashMap<Path, Path> sBackups;
  public:
    static constexpr const char* sSupportArchivePath = "/recalbox/share/saves/%FILE%";
    /*!
     * @brief Get HTTP method name
     * @param method Method
     * @return Method name
     */
    static const char* MethodToString(Pistache::Http::Method method);

    /*!
     * @brief Log
     * @param request Request to information from
     * @param methodName Method called
     */
    static void LogRoute(const Pistache::Rest::Request& request, const char* methodName);

    /*!
     * @brief Send back a response
     * @param response Response object
     * @param code Http code
     * @param body Response Body
     * @param mime MIME type
     */
    static void Send(Pistache::Http::ResponseWriter& response, Pistache::Http::Code code, const String& body, const Pistache::Http::Mime::MediaType& mime);

    /*!
     * @brief Send back a response
     * @param response Response object
     * @param code Http code
     */
    static Pistache::Async::Promise<ssize_t> Send(Pistache::Http::ResponseWriter& response, Pistache::Http::Code code);

    /*!
     * @brief Execute external command
     * @param command Command to execute
     * @return Output lines
     */
    static String::List OutputLinesOf(const String& command);

    /*!
     * @brief Execute external command
     * @param command Command to execute
     * @return Output lines
     */
    static String OutputOf(const String& command);

    /*!
     * @brief Send a 404 error
     */
    static void Error404(Pistache::Http::ResponseWriter& response);

    /*!
     * @brief Check and send target resource if it exists or return 404
     * @param resourcepath Resource path
     * @param response Response object
     */
    static void SendResource(const Path& resourcepath, Pistache::Http::ResponseWriter& response, const Pistache::Http::Mime::MediaType& mimeType);

    /*!
     * @brief Check and send target resource if it exists or return 404
     * check and send preferedPath first, then fallback
     * @param preferedPath Prefered resource path
     * @param fallbackPath Fallback resource path if Prefered path does not exists
     * @param response Response object
     * @param mimetype Mime Type
     */
    static void SendResource(const Path& preferedPath, const Path& fallbackPath, Pistache::Http::ResponseWriter& response, const Pistache::Http::Mime::MediaType& mimetype);

    /*!
     * @brief Build system resource path, file-based
     * @param path Output file-based path
     * @param filename Target filename
     */
    static void GetSystemResourcePath(Path& path, const String& filename);

    /*!
     * @brief Make the boot partition allowed to write
     * @return
     */
    static bool MakeBootReadWrite();

    /*!
     * @brief Performs a reset factory
     */
    static bool ResetFactory();
};
