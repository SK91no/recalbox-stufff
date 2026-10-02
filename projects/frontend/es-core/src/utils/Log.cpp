#include "Log.h"
#include <unistd.h>
#include <fcntl.h>
#include "utils/datetime/DateTime.h"
#include "utils/os/system/Mutex.h"

bool Log::sLogsRotated = false;
// Log paths now live in the lazily-constructed Log::Paths() accessor (see Log.h)
// to avoid any static-initialization-order dependency.
LogLevel Log::sReportingLevel[sTypeCount] = { LogLevel::LogInfo, LogLevel::LogInfo };

const char* Log::sStringLevel[] =
{
  "FATAL",
  "ERROR",
  "WARN!",
  "INFO ",
	"DEBUG",
  "TRACE",
};

Log::Log(LogType type, LogLevel level, const char* func, int line)
  : mType(type)
  , mMessageLevel(level)
  , mFunction(func)
  , mLine(line)
{
  // Not thread safe but quick test
  if (!sLogsRotated)
  {
    static Mutex sLocker;
    Mutex::AutoLock lock(sLocker);
    // Check again under the mutex so that we ensure only once rotation is done
    if (!sLogsRotated)
    {
      sLogsRotated = true;
      RotateLogs();
    }
  }
}

void Log::SetAllReportingLevel(LogLevel level)
{
  for(int i = sTypeCount; --i >= 0;)
    sReportingLevel[i] = level;
}

void Log::SetAllMinimumReportingLevel(LogLevel level)
{
  for(int i = sTypeCount; --i >= 0;)
    if (sReportingLevel[i] < level)
      sReportingLevel[i] = level;
}

void Log::RotateLogs()
{
  for(int i = sTypeCount; --i >= 0;)
  {
    Path backup1 = Paths()[i].ChangeExtension(".backup");
    Path backup2 = Paths()[i].ChangeExtension(".backup-2");
    Path backup3 = Paths()[i].ChangeExtension(".backup-3");
    (void)backup3.Delete();
    Path::Rename(backup2, backup3);
    Path::Rename(backup1, backup2);
    Path::Rename(Paths()[i], backup1);
  }
}

Log& Log::get()
{
	mMessage.Append('[')
	        .Append(DateTime().ToPreciseTimeStamp())
	        .Append("] (")
	        .Append(sStringLevel[(int)mMessageLevel])
          .Append(") ");
          if (sReportingLevel[(int)mType] >= LogLevel::LogDebug)
          {
            char tmp[16]; tmp[0] = 0;
            String t;
            if (pthread_getname_np(pthread_self(), tmp, 32); tmp[0] != 0) t.Assign(tmp);
            else t.Assign(gettid());
            mMessage.Append('{').Append(t).Append(' ', 15 - t.Count()).Append("} ");
          }
	        mMessage.Append(": ");

	return *this;
}

Log::~Log()
{
  if (sReportingLevel[(int)mType] >= LogLevel::LogDebug)
  {
    String func(mFunction);
    mMessage.Append(" (", 2).Append(SimplifyMethod(func)).Append(':').Append(mLine).Append(')');
  }
  mMessage.Append('\n');
  int fd = open(Paths()[(int)mType].ToChars(), O_CREAT | O_APPEND | O_WRONLY | O_NOATIME | O_SYNC, 0666);
  if (fd >= 0)
  {
    write(fd, mMessage.data(), mMessage.size());
    close(fd);
  }
  else printf("*** Unable to log! *** %s", mMessage.c_str());

  if(mMessageLevel == LogLevel::LogError || sReportingLevel[(int)mType] >= LogLevel::LogDebug)
    (void)fputs(mMessage.c_str(), stdout);

  if (mMessageLevel == LogLevel::LogFatal)
    abort();
}

const String& Log::SimplifyMethod(String& method)
{
  if (int par = method.Find('('))
  {
    method.Delete(par, method.Count() - par);
    if (int space = (int)method.rfind(' '); space > 0) method.Delete(0, space + 1);
  }
  return method;
}
