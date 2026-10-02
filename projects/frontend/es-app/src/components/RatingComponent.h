#pragma once

#include <components/base/ThemableComponent.h>
#include "IRatingComponent.h"
#include <rendering/textures/Texture.h>

// Used to visually display/edit some sort of "score" - e.g. 5/10, 3/5, etc.
// setSize(x, y) works a little differently than you might expect:
//   * (0, y != 0) - x will be automatically calculated (5*y).
//   * (x != 0, 0) - y will be automatically calculated (x/5).
//   * (x != 0, y != 0) - you better be sure x = y*5
class RatingComponent : public ThemableComponent
{
  public:
    explicit RatingComponent(WindowManager&window, float value, const String& discriminent);
    explicit RatingComponent(WindowManager&window, unsigned int color, float value, const String& discriminent);

    [[nodiscard]] String getValue() const override { return String(mValue, 2); }
    void setValue(const String& value) override; // Should be a normalized float (in the range [0..1]) - if it's not, it will be clamped.
    void setValue(float value); // Should be a normalized float (in the range [0..1]) - if it's not, it will be clamped.

    bool ProcessInput(const InputCompactEvent& event) override;
    void Render(const Transform4x4f& parentTrans) override;

    void setColor(unsigned int color) override { mColor=color; }

    void setOriginColor(unsigned int color){mOriginColor = color;};
    unsigned int getOriginColor() override{return mOriginColor;};

    void onSizeChanged() override;

    bool CollectHelpItems(Help& help) override;

    void SetInterface(int id, IRatingComponent* interface)
    {
      mId = id;
      mInterface = interface;
    }

      /*
       * Themable implementation
       */

      /*!
       * @brief Apply theme element to this nine-patch image
       * @param element Theme element
       * @param properties Properties to set
       */
      void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

      /*!
       * @brief Return theme element type
       * @return Element type
       */
      [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Rating; }

  private:
    static constexpr int sRatingStarCount = 5;

    static String sFilledTexture;
    static String sUnfilledTexture;

    Texture mFilledTexture;
    Texture mUnfilledTexture;

    String mDiscriminent;
    float mValue;
    unsigned int mColor;
    unsigned int mOriginColor;
    int mId;
    IRatingComponent* mInterface;
};

