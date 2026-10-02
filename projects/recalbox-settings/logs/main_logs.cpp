#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <zconf.h>
//#include <cerrno>

void GetParameters(int argc, char** argv, char*& customfile, char*& script, char*& type, bool& unescape, char*& text, int& textlen)
{
  customfile = text = nullptr;
  unescape = false;

  for(int i = 1; i < argc; )
  {
    char* param = argv[i++];
    if (strcmp(param, "-f") == 0)
    {
      if (i < argc)
        customfile = argv[i++];
    }
    else if (strcmp(param, "-e") == 0)
    {
      unescape = true;
    }
    else if (strcmp(param, "-s") == 0)
    {
      if (i < argc)
        script = argv[i++];
    }
    else if (strcmp(param, "-t") == 0)
    {
      if (i < argc)
        type = argv[i++];
    }
    else
    {
      text = param;
      if (text[0] == '"') ++text;
      textlen = strlen(text);
      if (text[textlen - 1] == '"') --textlen;
    }
  }
}

#define LOG_PATH "/recalbox/share/system/logs/"
#define LOG_PATH_LEN (sizeof(LOG_PATH) - 1)

#define TEMP_LOG_PATH "/tmp/logappend-"
#define TEMP_LOG_PATH_LEN (sizeof(TEMP_LOG_PATH) - 1)

static const char* DefaultLogFile = "recalbox.log";
static       char  LogFilePath[2U << 10U]; // 2kb
static       char  TextBuffer[64U << 10U]; // 64kb

char* BuildLogPath(const char* logfile, bool temp)
{
  if (logfile == nullptr)
    logfile = DefaultLogFile;

  if (temp)
  {
    memcpy(LogFilePath, TEMP_LOG_PATH, TEMP_LOG_PATH_LEN);
    memcpy(&LogFilePath[TEMP_LOG_PATH_LEN], logfile, strlen(logfile) + 1);
  }
  else
  {
    memcpy(LogFilePath, LOG_PATH, LOG_PATH_LEN);
    memcpy(&LogFilePath[LOG_PATH_LEN], logfile, strlen(logfile) + 1);
  }

  return LogFilePath;
}

const char* UnEscapeString(const char* string, int& length)
{
  char* result = TextBuffer - 1;
  char c;
  for(--string; (c = *(++string)) != 0; *(++result) = c)
  {
    if (c == '\\')
    {
      c = *(++string);
      switch (c)
      {
        case '0': c = '\0'; break;
        case 'a': c = '\a'; break;
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        case 'v': c = '\v'; break;
        case '?': c = '\?'; break;
        case 0  : *(result + 1) = 0; return TextBuffer;
        default: break;
      }
    }
  }
  *(result + 1) = 0;
  return TextBuffer;
}

char* LogTime(int& length)
{
  constexpr int formattedSize = 12;
  static char uptime[formattedSize + 2 + 1 + 1]; // [formattedSize]<sp>\0

  memset(uptime, '-', sizeof(uptime));
  uptime[sizeof(uptime) - 1] = 0;
  uptime[sizeof(uptime) - 2] = ' ';
  uptime[sizeof(uptime) - 3] = ']';
  uptime[0] = '[';

  int fd = open("/proc/uptime", O_RDONLY);
  if (fd >= 0)
  {
    read(fd, &uptime[1], formattedSize);
    close(fd);
  }

  int space = 1;
  while (uptime[space] != ' ' and uptime[space] != ']') space++;
  int move = 1 + formattedSize - space;
  if (move != 0)
  {
    for (int i = space; --i > 0;)
      uptime[move + i] = uptime[i];
    for (int i = move + 1; --i > 0;)
      uptime[i] = ' ';
  }

  length = sizeof(uptime) - 1;
  return uptime;
}

void TempToRegularLogs(const char* targetfile, int fdd)
{
  const char* sourceFile = BuildLogPath(targetfile, true);
  int fds = open(sourceFile, O_RDONLY);
  if (fds >= 0)
  {
    for(int length; (length = read(fds, TextBuffer, sizeof(TextBuffer))) > 0; )
      write(fdd, TextBuffer, length);
    close(fds);
    remove(sourceFile);
  }
}

#define FILE_APPEND_FLAGS (int)((unsigned int)O_APPEND | (unsigned int)O_CREAT | (unsigned int)O_RDWR)

void writeInCrochette(int fd, const char* text)
{
  write(fd, "[", 1);
  write(fd, text, strlen(text));
  write(fd, "] ", 2);
}

void LogSimpleText(const char* targetfile, const char* script, const char* type, const char* text, int length, bool unescape)
{
  if (unescape)
    text = UnEscapeString(text, length);

  int fd = open(BuildLogPath(targetfile, false), FILE_APPEND_FLAGS);
  if (fd < 0)
    fd = open(BuildLogPath(targetfile, true), FILE_APPEND_FLAGS);
  else
    if (fd >= 0)
      TempToRegularLogs(targetfile, fd);

  if (fd >= 0)
  {
    int uptimeLength = 0;
    char* uptime = LogTime(uptimeLength);
    write(fd, uptime, uptimeLength);
    if(script)
      writeInCrochette(fd, script);
    if(type)
      writeInCrochette(fd, type);
    write(fd, text, length);
    write(fd, "\n", 1);
    close(fd);
  }
}

void LogStdin(const char* targetfile, const char* script, const char* type, bool unescape)
{
  const char* text = TextBuffer;

  int fd = open(BuildLogPath(targetfile, false), FILE_APPEND_FLAGS);
  if (fd < 0)
    fd = open(BuildLogPath(targetfile, true), FILE_APPEND_FLAGS);
  else
    if (fd >= 0)
      TempToRegularLogs(targetfile, fd);

  if (fd >= 0)
  {
    int uptimeLength = 0;
    char* uptime = LogTime(uptimeLength);
    while(fgets(TextBuffer, sizeof(TextBuffer), stdin))
    {
      write(fd, uptime, uptimeLength);
      if(script)
        writeInCrochette(fd, script);
      if(type)
        writeInCrochette(fd, type);
      int lineLength = strlen(text);
      if (unescape)
        text = UnEscapeString(text, lineLength);
      write(fd, text, lineLength);
    }
    close(fd);
  }
}

//#include <exception>

int main(int argc, char** argv)
{
  char* TargetFile = nullptr;
  char* Text = nullptr;
  char* Script = nullptr;
  char* Type = nullptr;
  int Length = 0;
  bool Unescape = false;

  GetParameters(argc, argv, TargetFile, Script, Type, Unescape, Text, Length);

  if (Text != nullptr)
    LogSimpleText(TargetFile, Script, Type, Text, Length, Unescape);
  else
    LogStdin(TargetFile, Script, Type, Unescape);

  return 0;
}
