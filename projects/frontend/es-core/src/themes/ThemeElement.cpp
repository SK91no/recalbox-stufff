//
// Created by bkg2k on 14/10/25.
//

#include <themes/ThemeElement.h>
#include <themes/ThemeManager.h>

namespace
{
  ThemeStringPool& Strings() { return ThemeManager::Instance().ThemeStrings(); }
  ThemePropertyBagPool& Bags() { return ThemeManager::Instance().ThemeBags(); }
}

int ThemeElement::FindBag(ThemePropertyName name) const
{
  // Double-ended linear scan: properties per element are few, so this beats a hashmap
  for (int i = 0, j = mProperties.Count() - 1; i <= j; ++i, --j)
  {
    if (mProperties[i].mName == name) return mProperties[i].mBagIndex;
    if (mProperties[j].mName == name) return mProperties[j].mBagIndex;
  }
  return -1;
}

void ThemeElement::SetBag(ThemePropertyName name, int bagIndex)
{
  for (int i = mProperties.Count(); --i >= 0;)
    if (mProperties[i].mName == name) { mProperties(i).mBagIndex = bagIndex; return; }
  mProperties.Add({ name, bagIndex });
}

String ThemeElement::AsString(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 ? Bags().Get(idx).AsString(Strings(), mVariables) : String::Empty;
}

Path ThemeElement::AsPath(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 ? Bags().Get(idx).AsPath(Strings(), mVariables) : Path::Empty;
}

int ThemeElement::AsInt(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 ? Bags().Get(idx).AsInt(Strings(), mVariables) : 0;
}

float ThemeElement::AsFloat(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 ? Bags().Get(idx).AsFloat(Strings(), mVariables) : 0.f;
}

bool ThemeElement::AsBool(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 && Bags().Get(idx).AsBool(Strings(), mVariables);
}

Vector2f ThemeElement::AsVector(ThemePropertyName name) const
{
  int idx = FindBag(name);
  return idx >= 0 ? Bags().Get(idx).AsVector(Strings(), mVariables) : Vector2f(0.0f, 0.0f);
}

Alignment ThemeElement::AsAlignment(ThemePropertyName name) const
{
  static HashMap<String, Alignment> sStringToAlignment
  {
    { "left"         , Alignment::CenterLeft },
    { "right"        , Alignment::CenterRight },
    { "top"          , Alignment::TopCenter },
    { "bottom"       , Alignment::BottomCenter },
    { "topleft"      , Alignment::TopLeft },
    { "topcenter"    , Alignment::TopCenter },
    { "topright"     , Alignment::TopRight },
    { "centerleft"   , Alignment::CenterLeft },
    { "center"       , Alignment::Center },
    { "centerright"  , Alignment::CenterRight },
    { "bottomleft"   , Alignment::BottomLeft },
    { "bottomcenter" , Alignment::BottomCenter },
    { "bottomright"  , Alignment::BottomRight },
  };
  int idx = FindBag(name);
  if (idx < 0) return Alignment::CenterLeft;
  Alignment* result = sStringToAlignment.try_get(Bags().Get(idx).AsString(Strings(), mVariables).LowerCase());
  return result != nullptr ? *result : Alignment::CenterLeft;
}

void ThemeElement::AddVectorProperty(ThemePropertyName name, float x, float y)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeVector(x, y)));
  mAvailableProperties.Set(name);
}

void ThemeElement::AddVectorProperty(ThemePropertyName name, const Vector2f& v)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeVector(v.x(), v.y())));
  mAvailableProperties.Set(name);
}

void ThemeElement::AddStringProperty(ThemePropertyName name, const String& s)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeString(Strings().Intern(s))));
  mAvailableProperties.Set(name);
}

void ThemeElement::AddVariableProperty(ThemePropertyName name, const String& s, ThemeContexts context)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeUnresolved(Strings().Intern(s), context)));
  mAvailableProperties.Set(name);
  mMergedContext |= context;
}

void ThemeElement::AddIntProperty(ThemePropertyName name, int v)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeInt(v)));
  mAvailableProperties.Set(name);
}

void ThemeElement::AddFloatProperty(ThemePropertyName name, float v)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeFloat(v)));
  mAvailableProperties.Set(name);
}

void ThemeElement::AddBoolProperty(ThemePropertyName name, bool v)
{
  SetBag(name, Bags().Intern(PropertyBag::MakeBool(v)));
  mAvailableProperties.Set(name);
}
