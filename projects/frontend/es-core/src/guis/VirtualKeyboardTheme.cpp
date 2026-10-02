//
// Virtual keyboard theme resolver implementation.
//
#include "VirtualKeyboardTheme.h"
#include <themes/ThemeManager.h>
#include <themes/ThemeData.h>
#include <themes/ThemeElement.h>

void VirtualKeyboardTheme::LoadFromView(const String& view)
{
  LoadFromTheme(ThemeManager::Instance().Main(), view);
}

void VirtualKeyboardTheme::LoadFromTheme(const ThemeData& theme, const String& view)
{
  const ThemeElement* e = theme.Element(view, "keyboard", ThemeElementType::Keyboard, ThemeElementType::None);
  if (e == nullptr) return;

  if (e->HasProperty(ThemePropertyName::KeyColor))               KeyColor               = (unsigned int)e->AsInt(ThemePropertyName::KeyColor);
  if (e->HasProperty(ThemePropertyName::KeySelectedColor))       KeySelectedColor       = (unsigned int)e->AsInt(ThemePropertyName::KeySelectedColor);
  if (e->HasProperty(ThemePropertyName::KeyTextColor))           KeyTextColor           = (unsigned int)e->AsInt(ThemePropertyName::KeyTextColor);
  if (e->HasProperty(ThemePropertyName::KeyDisabledColor))       KeyDisabledColor       = (unsigned int)e->AsInt(ThemePropertyName::KeyDisabledColor);
  if (e->HasProperty(ThemePropertyName::KeyModifierColor))       KeyModifierColor       = (unsigned int)e->AsInt(ThemePropertyName::KeyModifierColor);
  if (e->HasProperty(ThemePropertyName::KeyModifierLockedColor)) KeyModifierLockedColor = (unsigned int)e->AsInt(ThemePropertyName::KeyModifierLockedColor);
  if (e->HasProperty(ThemePropertyName::KeyTitleColor))          TitleColor             = (unsigned int)e->AsInt(ThemePropertyName::KeyTitleColor);
  if (e->HasProperty(ThemePropertyName::KeyEditTextColor))       EditTextColor          = (unsigned int)e->AsInt(ThemePropertyName::KeyEditTextColor);

  if (e->HasProperty(ThemePropertyName::FontPath))
  {
    FontPath = e->AsPath(ThemePropertyName::FontPath);
    HasFont = !FontPath.IsEmpty();
  }
}
