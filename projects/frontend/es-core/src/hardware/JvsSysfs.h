//
// Created by digitalLumberjack on 2026-06-09.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
// Thin R/W wrapper over /sys/devices/platform/recalboxjvs/<attr>. The kernel
// module pushes/pulls the firmware NVS over HID Feature Report 5; userspace
// just talks to sysfs and the kernel handles the round-trip.
//
#pragma once

#include <utils/Files.h>
#include <utils/os/fs/Path.h>
#include <utils/String.h>
#include <cstdio>

class JvsSysfs
{
  public:
    //! True if the recalboxjvs platform device is bound (module loaded + USB up).
    static bool Available()
    {
      return Path(sBase).Exists();
    }

    //! Read a 0/1 attribute. Returns false on any error or absent attr.
    static bool ReadBool(const char* attr)
    {
      return Files::LoadFile(AttrPath(attr)).Trim() == "1";
    }

    //! Read an integer attribute. Returns dflt on any error.
    static int ReadInt(const char* attr, int dflt = 0)
    {
      String s = Files::LoadFile(AttrPath(attr)).Trim();
      if (s.empty()) return dflt;
      int v = 0;
      if (sscanf(s.c_str(), "%d", &v) != 1) return dflt;
      return v;
    }

    //! Read a free-form text attribute (e.g. cab_type=joystick/race/gun).
    static String ReadString(const char* attr)
    {
      return Files::LoadFile(AttrPath(attr)).Trim();
    }

    static bool WriteBool(const char* attr, bool value)
    {
      return WriteString(attr, value ? "1" : "0");
    }

    static bool WriteInt(const char* attr, int value)
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%d", value);
      return WriteString(attr, buf);
    }

    static bool WriteString(const char* attr, const char* value)
    {
      return Files::SaveFile(AttrPath(attr), String(value));
    }

  private:
    static constexpr const char* sBase = "/sys/devices/platform/recalboxjvs";

    static Path AttrPath(const char* attr)
    {
      return Path(String(sBase).Append('/').Append(attr));
    }
};
