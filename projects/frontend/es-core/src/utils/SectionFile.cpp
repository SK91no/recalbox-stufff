//
// Created by bkg2k on 18/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//

#include "SectionFile.h"
#include <utils/Files.h>
#include <utils/Log.h>

SectionFile::SectionFile(const Path& filePath, const Path& fallbackFilePath, bool spaceAroundSeparator, ISectionFileSpy* spy)
  : mPrimary(filePath)
  , mFallback(fallbackFilePath)
  , mSpy(spy)
  , mExtraSpace(spaceAroundSeparator)
  , mLoadedFromFallBack(false)
{
  Load();
}

SectionFile::Section& SectionFile::GetOrCreateSection(const String& sectionKey)
{
  // Default section?
  if (sectionKey.empty()) return mDefault;
  // Already exist?
  Section* section = mSections.try_get(sectionKey);
  // Yes, get it
  if (section != nullptr) [[likely]] return *section;
  // No, create it
  return mSections[sectionKey];
}

void SectionFile::Load()
{
  bool loaded = false;
  if (!mPrimary.IsEmpty())
  {
    loaded = Load(mPrimary, false);
    { LOG(LogDebug) << "[SectionFile] Primary file " << mPrimary.ToString() << (loaded ? " loaded successfully" : " not loaded!"); }
  }
  if (!loaded && !mFallback.IsEmpty())
  {
    mLoadedFromFallBack = loaded = Load(mFallback, true);
    { LOG(LogDebug) << "[SectionFile] Fallback file " << mFallback.ToString() << (loaded ? " loaded successfully" : " not loaded!"); }
  }
  if (mSpy != nullptr)
    mSpy->OnSectionFileLoad(loaded);
}

bool SectionFile::LoadLines(const Path& filePath, String::List& to)
{
  if (!filePath.Exists()) return false;
  // Load files
  String content = Files::LoadFile(filePath).Remove('\r');
  // Slit lines
  to = content.Split('\n');
  return true;
}

bool SectionFile::SaveLines(const Path& filePath, String::List& from)
{
  // Join lines
  String content = String::Join(from,LEGACY_STRING("\r\n"));
  // Save files
  return Files::SaveFile(filePath, content);
}

bool SectionFile::Load(const Path& filePath, bool keepExistingValues)
{
  // Load file
  String::List lines;
  if (!LoadLines(filePath, lines)) return false;

  // Import
  Import(lines, keepExistingValues);

  return true;
}

bool SectionFile::Save(const Path& filePath)
{
  // Load lines first
  String::List lines;
  LoadLines(mLoadedFromFallBack ? mFallback : mPrimary, lines);

  // Export
  Export(lines);

  // Then save
  return SaveLines(filePath, lines);
}

void SectionFile::Export(String::List& to)
{
  // Export the default section first
  int index = mDefault.Export(to, 0, mExtraSpace);

  // Then next sections
  while(index < (int)to.size())
  {
    String sectionName;
    if (!Section::ExtractSection(to[index], sectionName)) break; // Garbage?
    index = GetOrCreateSection(sectionName).Export(to, ++index, mExtraSpace);
  }

  // Now there might be extra section that do not appear in the original list
  for(auto& kvs : mSections)
    if (kvs.second.HasPendings())
    {
      // Add an empty line before if the last line is not empty already
      if (!to.empty())
        if (!to[to.size() - 1].Trim().empty()) to.push_back(String());
      // Then add section name
      to.push_back(String('[').Append(kvs.first).Append(']'));
      // Then export as usual. Only new values will be exported since there is nothing to modify
      kvs.second.Export(to, (int)to.size(), mExtraSpace);
    }
}

bool SectionFile::Import(const String::List& from, bool keepExistingValues)
{
  // Browse lines
  Section* section = &mDefault;
  String sectionName;
  double globalErrorLevel = 0.;
  for(int i = 0; i < (int)from.size(); )
  {
    // Section entry?
    if (Section::ExtractSection(from[i], sectionName))
    {
      section = &GetOrCreateSection(sectionName);
      i++; // move next line
    }

    // Load section's content
    double sectionErrorLevel = 0;
    int end = section->Import(from, i, sectionErrorLevel, keepExistingValues);
    globalErrorLevel += sectionErrorLevel * double(end - i);
    i = end;
  }

  // Only 1% of error is acceptable
  globalErrorLevel /= (double)from.size();
  return (globalErrorLevel <= 0.01);
}

int SectionFile::Section::Import(const String::List& from, int index, double& errorLevel, bool keepExistingValues)
{
  String key, value;
  int startIndex = index;
  errorLevel = 0.0;
  for(; index < (int)from.size(); ++index)
  {
    bool exit = false;
    switch(ParseLine(from[index], key, value))
    {
      case LineType::ActiveKeyValue:
      {
        if (!mKeyValues.contains(key) || !keepExistingValues)
          mKeyValues[key] = value;
        break;
      }
      case LineType::SectionHeader: exit = true; break;
      case LineType::Unidentified: errorLevel += 0.5; break;
      case LineType::Binary: errorLevel += 1.0; break;
      case LineType::TooLong: errorLevel += 0.5; break;
      case LineType::InvalidKey: errorLevel += 0.25; break;
      case LineType::InactiveKeyValue:
      case LineType::Blank:
      case LineType::Comment:
      default: break;
    }
    if (exit) break; // break for loop
  }

  // Adjust error level
  if (index > startIndex)
    errorLevel /= (double)(index - startIndex);

  return index;
}

int SectionFile::Section::Export(String::List& to, int index, bool extraSpace)
{
  index = FlushPending(to, index, extraSpace);
  index = FlushNew(to, index, extraSpace);
  RemoveUnflushed();
  return index;
}

int SectionFile::Section::FlushPending(String::List& to, int index, bool extraSpace)
{
  // Flush modified and deleted keys into existing values
  // Deleted keys are commented using ';' so that further reintroduction
  // will happen in their former location
  String key, value;
  for(; index < (int)to.size(); ++index)
  {
    bool exit = false;
    switch(ParseLine(to[index], key, value))
    {
      case LineType::ActiveKeyValue:
      case LineType::InactiveKeyValue:
      {
        // Deleted?
        if (mPendingDelete.contains(key))
        {
          to[index].Insert(0, ';');
          mPendingDelete.erase(key);
          mPendingWrites.erase(key);
          mKeyValues.erase(key);
        }
        else if (String* newValue = mPendingWrites.try_get(key); newValue != nullptr)
        {
          if (extraSpace) to[index].Assign(key).Append(LEGACY_STRING(" = ")).Append(*newValue);
          else to[index].Assign(key).Append('=').Append(*newValue);
          mKeyValues[key] = *newValue;
          mPendingWrites.erase(key);
        }
        break;
      }
      case LineType::SectionHeader: exit = true; break;
      case LineType::Unidentified:
      case LineType::Blank:
      case LineType::Comment:
      case LineType::Binary:
      case LineType::TooLong:
      case LineType::InvalidKey:
      default: break;
    }
    if (exit) break; // break for loop
  }

  return index;
}

int SectionFile::Section::FlushNew(String::List& to, int index, bool extraSpace)
{
  // Here, we are requested to flush remaining keys that have not been located in the original list.
  // So, just do it, at the given index
  bool added = false;
  for(const auto& kv : mPendingWrites)
  {
    // Not deleted?
    if (mPendingDelete.contains(kv.first)) continue;
    // Insert in list
    if (extraSpace) to.insert(to.begin() + index, String(kv.first).Append(LEGACY_STRING(" = ")).Append(kv.second));
    else to.insert(to.begin() + index, String(kv.first).Append('=').Append(kv.second));
    // Copy modified value into the regular map
    mKeyValues[kv.first] = kv.second;
    ++index;
    added = true;
  }
  mPendingWrites.clear();

  // Add an empty lines if keys have been added
  bool emptyLine = (!to.empty() && index < (int)to.size()) ? to[index].Trim().empty() : false;
  if (added && !emptyLine) to.insert(to.begin() + index++, String());

  return index;
}


void SectionFile::Section::RemoveUnflushed()
{
  // Remove remaining deleted key.
  // If they are still deleted key, it means they have been added and then deleted since
  // they do not appear in the original list.
  // So, just delete values in maps
  for(const auto& k : mPendingDelete)
  {
    mPendingWrites.erase(k);
    mKeyValues.erase(k);
  }
  mPendingDelete.clear();
}

SectionFile::LineType SectionFile::Section::ParseLine(const String& line, String& leftOutput, String& rightOutput)
{
  // Optimized parsing routine. Everything is done in the String 'line' by playing with char index
  // The only required String operations are key/values extractions

  // Too long?
  if (line.Count() > 4096) return LineType::TooLong;
  // Contains binary?
  if (line.CountControl() != 0) return LineType::Binary;

  // Blank?
  int firstNonSpace = (int)line.find_first_not_of(" \t");
  if (firstNonSpace < 0) return LineType::Blank;
  // Comment?
  char firstChar = line[firstNonSpace];
  if (firstChar == '#') return LineType::Comment;

  // Section?
  if (firstChar == '[')
  {
    int lastNonSpace = (int)line.find_last_not_of(" \t");
    if (lastNonSpace > firstNonSpace && line[lastNonSpace] == ']') return LineType::SectionHeader;
    return LineType::Unidentified; // Mis-formatted section
  }

  // Key/value?
  if (firstChar == ';') ++firstNonSpace;
  if (int equal = line.Find('=', firstNonSpace); equal > firstNonSpace)
    if (int lastNonSpace = (int)line.find_last_not_of(" \t", equal - 1); lastNonSpace > firstNonSpace)
    {
      if (IsValidIdentifier(line, firstNonSpace, lastNonSpace))
      {
        leftOutput = String(line.data() + firstNonSpace, lastNonSpace - firstNonSpace + 1);
        rightOutput = String(line.data() + equal + 1, line.Count() - (equal + 1)).Trim();
        return firstChar == ';' ? LineType::InactiveKeyValue : LineType::ActiveKeyValue;
      }
      else return LineType::InvalidKey;
    }

  // Unidentified yet...
  return LineType::Unidentified;
}
