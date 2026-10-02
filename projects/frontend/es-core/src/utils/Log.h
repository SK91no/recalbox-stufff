#ifndef _LOG_H_
#define _LOG_H_

#include <utils/String.h>
#include <utils/os/fs/Path.h>

//! Log types
enum class LogType
{
  Generic, //!< Default logs
  Themes,  //!< Theme logs
  __Count,
};

static constexpr const int sTypeCount = (int)LogType::__Count;

#define LOG(level) \
if (::LogLevel::level <= ::Log::ReportingLevel(LogType::Generic)) ::Log(LogType::Generic, LogLevel::level, __PRETTY_FUNCTION__, __LINE__).get()

#define LOGT(level) \
if (::LogLevel::level <= ::Log::ReportingLevel(LogType::Themes)) ::Log(LogType::Themes, LogLevel::level, __PRETTY_FUNCTION__, __LINE__).get()

//! Log level
enum class LogLevel
{
  LogFatal   , //!< Fatal Error messages
	LogError   , //!< Error messages
	LogWarning , //!< Warning messages
	LogInfo    , //!< Information message
	LogDebug   , //!< Debug message
  LogTrace   , //!< Trace message (Internal debug!)
  _Count
};

class Log
{
  public:
    Log(LogType type, LogLevel level, const char* func, int line);
    ~Log();
    Log& get();

    static void SetPath(LogType type, const Path& path) { Paths()[(int)type] = path; }
    static const char* PathChars(LogType type) { return Paths()[(int)type].ToChars(); }
    static LogLevel ReportingLevel(LogType type) { return sReportingLevel[(int)type]; }
    static void SetReportingLevel(LogType type, LogLevel level) { sReportingLevel[(int)type] = level; }
    static void SetAllReportingLevel(LogLevel level);
    static void SetAllMinimumReportingLevel(LogLevel level);

    Log& operator << (char v) { mMessage.Append(v); return *this; }
    Log& operator << (const char* v) { mMessage.Append(v); return *this; }
    Log& operator << (const String& v) { mMessage.Append(v); return *this; }
    Log& operator << (const Path& v) { mMessage.Append(v.ToString()); return *this; }
    Log& operator << (int v) { mMessage.Append(v); return *this; }
    Log& operator << (unsigned int v) { mMessage.Append(v); return *this; }
    Log& operator << (long long v) { mMessage.Append(v); return *this; }
    Log& operator << (unsigned long long v) { mMessage.Append(v); return *this; }
    Log& operator << (long v) { mMessage.Append((long long)v); return *this; }
    Log& operator << (unsigned long v) { mMessage.Append((unsigned long long)v); return *this; }
    Log& operator << (bool v) { mMessage.Append(v); return *this; }
    Log& operator << (float v) { mMessage.Append(v, 4); return *this; }
    Log& operator << (const String::List& v) { for(const String& s : v) mMessage.Append(s).Append(' '); return *this; }
    Log& operator << (const std::vector<const char*>& v) { for(const char* s : v) mMessage.Append(s).Append(' '); return *this; }

  private:
    static bool sLogsRotated;
    static const char* sStringLevel[(int)LogLevel::_Count];
    static FILE* sFile;
    static LogLevel sReportingLevel[sTypeCount];

    //! Log file paths, constructed on first use (Meyers singleton). Routing the
    //! paths through a function-local static removes any static-initialization-
    //! order dependency: the array is built the first time it is accessed rather
    //! than at an unspecified point during global construction. This makes the
    //! paths safe to read from an early pre-main constructor — DeathTrap arms its
    //! crash handler before main() and needs a valid log path at that point.
    static Path* Paths()
    {
      static Path paths[sTypeCount] =
      {
        Path("/recalbox/share/system/logs/frontend.log"),
        Path("/recalbox/share/system/logs/themes.log"),
      };
      return paths;
    }

    LogType mType;
    LogLevel mMessageLevel;
    String mMessage;
    const char* mFunction;
    int mLine;

    static void RotateLogs();
    static const String& SimplifyMethod(String& method);
};

#endif
