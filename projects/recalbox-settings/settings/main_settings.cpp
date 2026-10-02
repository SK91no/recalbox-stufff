#include <cstdio>
#include <cstring>

enum class Command
{
  cmdUnknown,
  cmdLoad,
  cmdSave,
  cmdDisable,
};

static char SettingsBuffer[1U << 20U]; // 1Mb buffer
static int SettingsLength = 0;

#ifdef DEBUG
static char Debug[1U << 20U]; // 1Mb buffer
#endif

static const char* SettingsFile = "/recalbox/share/system/recalbox.conf";

Command GetParameters(int argc, char** argv, char*& key, char*& value, char*& defaultValue, const char*& source)
{
  key = value = defaultValue = nullptr;
  Command result = Command::cmdUnknown;

  source = SettingsFile;

  for(int i = 1; i < argc; )
  {
    char* param = argv[i++];
    if (strcmp(param, "-command") == 0)
    {
      if (i + 1 < argc)
      {
        param = argv[i++];
        if (strcmp(param, "load") == 0) result = Command::cmdLoad;
        else if (strcmp(param, "save") == 0) result = Command::cmdSave;
        else if (strcmp(param, "disable") == 0) result = Command::cmdDisable;
      }
    }
    else if (strcmp(param, "-key") == 0)
    {
      if (i < argc)
        key = argv[i++];
    }
    else if (strcmp(param, "-value") == 0)
    {
      if (i < argc)
        value = argv[i++];
    }
    else if (strcmp(param, "-default") == 0)
    {
      if (i < argc)
        defaultValue = argv[i++];
    }
    else if (strcmp(param, "-source") == 0)
    {
      if (i < argc)
        source = argv[i++];
    }
  }

  return result;
}

bool LoadSettings(const char* source)
{
  SettingsLength = 0;
  SettingsBuffer[0] = 0;

  FILE* f = fopen(source, "rb");
  if (f != nullptr)
  {
    fseek(f, 0, SEEK_END);
    SettingsLength = (int)ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(SettingsBuffer, SettingsLength, 1, f);
    fclose(f);
    SettingsBuffer[SettingsLength] = 0;
    return true;
  }
  return false;
}

bool SaveSettings()
{
  FILE* f = fopen(SettingsFile, "wb");
  if (f != nullptr)
  {
    fwrite(SettingsBuffer, SettingsLength, 1, f);
    fclose(f);
    return true;
  }
  return false;
}

bool Lookup(const char* key, int& start, int &sep, int& length, bool& enabled)
{
  if (key == nullptr) return false;
  const int keySize = strlen(key);
  if (keySize == 0) return false;

  int lineStart = 0;
  int lineStop;
  char* p = SettingsBuffer;

  // Loop through lines
  while(lineStart < SettingsLength)
  {
    // Get lines
    int separator = 0;
    for(lineStop = lineStart; lineStop < SettingsLength; lineStop++)
    {
      char c = p[lineStop];
      if (separator == 0)
        if (c == '=') separator = lineStop;
      if (c == 10 || c == 13) break;
    }

    // Separator exists?
    if (separator > lineStart) // if a line starts with an equal sign, ignore it
    {
      #ifdef DEBUG
        memset(Debug, 0, sizeof(Debug));
        char* dbg = Debug;
        strncpy(dbg, &p[lineStart], lineStop - lineStart);
      #endif

      int keyStart = lineStart;
      int keyStop = separator - 1;
      // At least one char
      if (keyStart < keyStop)
      {
        // Trim start of key
        while (keyStart < keyStop && (p[keyStart] == 32 || p[keyStart] == 9))
          keyStart++;
        if (keyStart < keyStop) // Still need one char
        {
          // Disabled?
          enabled = true;
          if (p[keyStart] == ';')
          {
            enabled = false;
            keyStart++;
            // Trim again
            while (keyStart < keyStop && (p[keyStart] == 32 || p[keyStart] == 9))
              keyStart++;
          }
          // Trim end of key
          while (keyStop > keyStart && (p[keyStop] == 32 || p[keyStop] == 9))
            keyStop--;
          // Still valid?
          if (keyStart < keyStop)
            if (keyStop - keyStart == keySize - 1)
              if (strncmp(key, &p[keyStart], keySize) == 0)
              {
                start = lineStart;
                length = lineStop - lineStart;
                sep = separator;
                return true;
              }
        }
      }
    }

    // Next line
    lineStart = lineStop;
    while(p[lineStart] == 10 || p[lineStart] == 13) lineStart++;
  }

  return false;
}

void Insert(int pos, int size)
{
  memmove(&SettingsBuffer[pos + size], &SettingsBuffer[pos], SettingsLength - pos);
  SettingsLength += size;
  SettingsBuffer[SettingsLength] = 0;
}

void Remove(int pos, int size)
{
  memmove(&SettingsBuffer[pos], &SettingsBuffer[pos + size], SettingsLength - (pos + size));
  SettingsLength -= size;
  SettingsBuffer[SettingsLength] = 0;
}

void Append(const char* value, int size)
{
  memcpy(&SettingsBuffer[SettingsLength], value, size);
  SettingsLength += size;
  SettingsBuffer[SettingsLength] = 0;
}

void Replace(int pos, int length, const char* newValue)
{
  int newLength = strlen(newValue);
  int diff = newLength - length;
  if (diff > 0) Insert(pos, diff);
  else if (diff < 0) Remove(pos, -diff);
  memcpy(&SettingsBuffer[pos], newValue, newLength);
}

const char* GetValue(int pos, int length)
{
  char* p = &SettingsBuffer[pos];
  // Cut & trim
  p[length] = 0; // DO NOT SAVE THE FILE AFTER CALLING THIS METHOD
  for(int i = length; --i >= 0; )
    if (p[i] == ' ') p[i] = 0;
    else break;
  // Lookup value
  for(int i = length; --i >= 0; )
    if (*p++ == '=')
    {
      for(int j = i; --j >= 0; p++)
        if (*p != ' ')
          break;
      return p;
    }
  return nullptr;
}

const char* LoadKey(const char* key, const char* defaultValue, const char* source)
{
  if (LoadSettings(source))
  {
    int start = 0;
    int sep = 0;
    int length = 0;
    bool enabled = false;

    if (Lookup(key, start, sep, length, enabled))
      if (enabled)
      {
        const char* value = GetValue(start, length);
        if (value != nullptr)
          return value;
      }
  }
  return defaultValue != nullptr ?  defaultValue : "";
}

void SaveKey(const char* key, const char* value, const char* source)
{
  if (LoadSettings(source))
  {
    int start = 0;
    int sep = 0;
    int length = 0;
    bool enabled = false;

    if (value == nullptr)
      value = "";

    // Key exists?
    if (Lookup(key, start, sep, length, enabled))
    {
      sep++;
      if (!enabled)
      {
        // If peviously disabled, re-enable
        Remove(start, 1);
        length--;
        sep--;
      }
      int valueLength = length - (sep - start);
      if (valueLength >= 0)
      Replace(sep, valueLength, value);
    }
    else
    {
      // Key does not exists, append
      Append("\r\n", 2);
      Append(key, strlen(key));
      Append("=", 1);
      Append(value, strlen(value));
      Append("\r\n", 2);
    }

    SaveSettings();
  }
}

void DisableKey(const char* key, const char* source)
{
  if (LoadSettings(source))
  {
    int start = 0;
    int sep = 0;
    int length = 0;
    bool enabled = false;

    if (Lookup(key, start, sep, length, enabled))
      if (enabled)
      {
        Replace(start, 0, ";");
        SaveSettings();
      }
  }
}

int main(int argc, char** argv)
{
  char* key = nullptr;
  char* value = nullptr;
  char* defaultValue = nullptr;
  const char* source = nullptr;

  Command command = GetParameters(argc, argv, key, value, defaultValue, source);

  switch(command)
  {
    case Command::cmdLoad:
    {
      puts(LoadKey(key, defaultValue, source));
      break;
    }
    case Command::cmdSave:
    {
      SaveKey(key, value, source);
      break;
    }
    case Command::cmdDisable:
    {
      DisableKey(key, source);
      break;
    }
    case Command::cmdUnknown:
    default: break;
  }

  return 0;
}