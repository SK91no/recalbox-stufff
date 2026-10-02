//
// Created by bkg2k on 11/12/23.
//

#include "BoxComponent.h"
#include <themes/ThemeData.h>
#include <Renderer.h>

void BoxComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Renderer::DrawRectangle(0.f, 0.f, mSize.x(), mSize.y(), mTopLeftColor, mTopRightColor, mBottomRightColor, mBottomLeftColor);
}

void BoxComponent::OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties)
{
  if (hasFlag(properties, ThemePropertyCategory::Color))
  {
    bool set = false;
    if (element.HasProperty(ThemePropertyName::Color)) { SetColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::Color)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTop)) { SetTopColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTop)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottom)) { SetBottomColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottom)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorLeft)) { SetLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorRight)) { SetRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorRight)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTopLeft)) { SetTopLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTopLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottomRight)) { SetBottomRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottomRight)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorBottomLeft)) { SetBottomLeftColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorBottomLeft)); set = true; }
    if (element.HasProperty(ThemePropertyName::ColorTopRight)) { SetTopRightColor((Colors::ColorRGBA)element.AsInt(ThemePropertyName::ColorTopRight)); set = true; }
    if (!set) setColor(0); // No color = full tranparent
  }
}
