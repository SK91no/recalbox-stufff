//
// Created by bkg2k on 13/10/23.
//
#pragma once

#include "components/base/Component.h"
#include <utils/String.h>
#include <memory>
#include "rendering/textures/Texture.h"
#include "ImgProps.h"
#include <components/base/ThemableComponent.h>

class PictureComponent : public ThemableComponent
{
  public:
    PictureComponent(WindowManager&window, const Path& imagePath, ImgProps props, int marginH, int marginV);

    PictureComponent(WindowManager&window, const Path& imagePath, ImgProps props)
      : PictureComponent(window, imagePath, props, 0, 0)
    {
    }

    PictureComponent(WindowManager&window, ImgProps props, int marginH, int marginV)
      : PictureComponent(window, Path::Empty, props, marginH, marginV)
    {
    }

    PictureComponent(WindowManager&window, ImgProps props)
      : PictureComponent(window, Path::Empty, props, 0, 0)
    {
    }

    explicit PictureComponent(WindowManager& window, int marginH, int marginV)
      : PictureComponent(window, Path::Empty, ImgProps::None, marginH, marginV)
    {
    }

    explicit PictureComponent(WindowManager& window)
      : PictureComponent(window, Path::Empty, ImgProps::None, 0, 0)
    {
    }

    ~PictureComponent() override = default;

    //! Set size in function of parent size
    void setNormalisedSize(float width, float height)
    {
      Vector2f pos = denormalise(width, height);
      setSize(pos.x(), pos.y());
    }

    //Loads the image at the given filepath. Will tile if tile is true (retrieves texture as tiling, creates vertices accordingly).
    void setImage(const Path& path, bool tile = false);
    [[nodiscard]] Path getImage() const { return mPath; };

    //! Remove image
    void ClearImage() { setImage(Path::Empty); }

    void setOpacity(unsigned char opacity) override;

    /*!
     * @brief Set keep ratrio
     * @param keepRatio True to keep image ratio, falser to stretch
     */
    void setKeepRatio(bool keepRatio)
    {
      bool hasKeepRatio = hasFlag(mProps, ImgProps::KeepRatio);
      if (hasKeepRatio != keepRatio)
      {
        mProps &= ~ImgProps::KeepRatio;
        if (keepRatio) mProps |= ImgProps::KeepRatio;
      }
    }

    void setColor(unsigned int color) override;

    void SetMargins(int h, int v) { mMarginH = h; mMarginV = v; }

    void Render(const Transform4x4f& parentTrans) override;

    /*!
     * @brief Set component visibility
     * @param enabled True (default) to render the component, false to hide it
     */
    void SetVisible(bool visible) { mVisible = visible; }

    /*!
     * @brief Set component visibility
     * @param enabled True (default) to render the component, false to hide it
     */
    [[nodiscard]] bool Visible() const { return mVisible; }

    /*!
     * @brief Apply theme element to this image
     * @param element Theme element
     * @param properties Properties to set
     */
    void OnApplyThemeElement(const ThemeElement& element, ThemePropertyCategory properties) override;

    /*!
     * @brief Return theme element type
     * @return Element type
     */
    [[nodiscard]] ThemeElementType GetThemeElementType() const override { return ThemeElementType::Image; }

  private:
    Path mPath;
    Texture mTexture;
    int mMarginH;
    int mMarginV;
    unsigned int mColorShift;
    ImgProps mProps;
    unsigned char mFadeOpacity;
    bool mFading;
    bool mVisible;
    bool mTiled;
    bool mPathChanged;

    bool fadeIn(bool textureLoaded);
};

