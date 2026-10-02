//
// Created by bkg2k on 30/06/26.
//

#include <themes/ThemePropertyBag.h>
#include <utils/Log.h>
#include <Renderer.h>

bool PropertyBag::ConvertPair(const String& string, float& x, float& y)
{
  String left;
  String right;
  if (string.ToTrim().Extract(' ', left, right, false))
  {
    bool hasX = left.TryAsFloat(0, 0, x);
    if (!hasX) if (hasX = left.TryAsFloat(0, '%', x); hasX) x /= 100.f;
    if (!hasX) if (hasX = left.TryAsFloat(0, 'p', x); hasX) x /= Renderer::Instance().DisplayWidthAsFloat();
    if (!hasX) { LOGT(LogError) << "[Theme] invalid 1st value in normalized pair"; return false; }
    bool hasY = right.TryAsFloat(0, 0, y);
    if (!hasY) if (hasY = right.TryAsFloat(0, '%', y); hasY) y /= 100.f;
    if (!hasY) if (hasY = right.TryAsFloat(0, 'p', y); hasY) y /= Renderer::Instance().DisplayHeightAsFloat();
    if (!hasY) { LOGT(LogError) << "[Theme] invalid 2nd value in normalized pair"; return false; }
    return true;
  }
  return false;
}
