//
// Created by bkg2k on 27/06/25.
//

#include "ThemeVariablesHolder.h"
#include <utils/Log.h>

void ThemeVariablesHolder::ResolveUserVariables()
{
  for(const auto& kv : mVariables)
  {
    ThemeUserVariable* userVar = mVariables.try_get(kv.first);
    String& source = userVar->mValue;

    // Replace all user variables first
    for (int start = 0, replace = 0;;)
    {
      int left = source.Find("${", 2, start); if (left < 0) break;
      int right = source.Find('}', left + 2); if (right < left) break;
      String variable = source.SubString(left + 2, right - (left + 2)).Trim();

      if (ThemeUserVariable* target = mVariables.try_get(variable); target != nullptr)
      {
        if (target == userVar) { source.Delete(left, right - left + 1).Insert(left, LEGACY_STRING("<ERROR: RECURSIVE VARIABLE>")); break; } // direct recursivity
        if (source.Count() >= (16 << 10)) { source.Delete(left, right - left + 1).Insert(left, LEGACY_STRING("<ERROR: OVERSIZED VARIABLE>")); break; } // oversize or indirect recursivity
        if (++replace > 256) { source.Delete(left, right - left + 1).Insert(left, LEGACY_STRING("<ERROR: TOO MUCH REPLACEMENTS IN VARIABLE>")); break; } // Indirect recursivity
        source.Delete(left, right - left + 1).Insert(left, target->mValue);
        start = left; // Restart at the exact replacement start point
        continue;
      }

      // Nothing to replace, just break
      break;
    }

    // Then analyze and replace other variables, except full-dynamic variables
    ThemeContexts context = mResolver.AnalyseContextualVariables(source);
    context &= ~(ThemeContexts::DisplayedSystemOffset | ThemeContexts::GameAndSystem);
    mResolver.ResolveContextualVariables(context, source, mSystem, nullptr);

    // Check
    context = mResolver.AnalyseContextualVariables(source);
    context &= ~(ThemeContexts::DisplayedSystemOffset | ThemeContexts::GameAndSystem); // Ignore dynamic variables
    if (mSystem == nullptr) context &= ~ThemeContexts::DisplayedSystem; // Ignore system variable when no system is available
    if (!isEmpty(context)) { LOGT(LogWarning) << "[UserVariablesHolder] Unresolved variables in " << userVar->Name() << " = " << userVar->Value(); }

    { LOGT(LogTrace) << "[UserVariablesHolder] Variable " << userVar->Name() << " = " << userVar->Value(); }
  }
}

void ThemeVariablesHolder::ResolveUserVariables(String& value) const
{
  for (int start = 0;;)
  {
    int left = value.Find("${", 2, start); if (left < 0) break;
    int right = value.Find('}', left + 2); if (right < left) break;
    String variable = value.SubString(left + 2, right - (left + 2)).Trim();

    if (ThemeUserVariable* target = mVariables.try_get(variable); target != nullptr)
    {
      value.Delete(left, right - left + 1).Insert(left, target->mValue);
      start = left + target->mValue.Count();
    }
    else start = right + 1;
  }
}

bool ThemeVariablesHolder::ValidateName(const String& name)
{
  if (name.empty()) return false;
  if (int c = (int)(unsigned char)name[0], cm = c | 0x20; (cm < 'a' || cm > 'z') && c != '_') return false;
  bool dot = false;
  for(int i = name.Count(); --i >= 1; )
    if (int c = (int)(unsigned char)name[i], cm = c | 0x20; (cm >= 'a' && cm <= 'z') || (c >= '0' && c <= '9') || (c == '_')) { dot = false; continue; }
    else if (c == '.' && !dot) { dot = true; continue; }
    else return false;
  return true;
}

void ThemeVariablesHolder::Add(const String& name, const String& value)
{
  if (ValidateName(name)) mVariables.insert_or_assign(name, ThemeUserVariable(name, value, mResolver));
  else { LOGT(LogError) << "[UserVariablesHolder] Invalid variable name " << name << ". Variable ignored."; }
}


