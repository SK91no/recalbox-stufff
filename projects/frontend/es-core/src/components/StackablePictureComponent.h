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

class StackablePictureComponent : public ThemableComponent
{
  public:
    StackablePictureComponent(WindowManager&window, const Path::PathList& imagePath, ImgProps props);

    StackablePictureComponent(WindowManager&window, ImgProps props)
      : StackablePictureComponent(window, Path::PathList(), props)
    {
    }

    explicit StackablePictureComponent(WindowManager& window)
      : StackablePictureComponent(window, Path::PathList(), ImgProps::None)
    {
    }

    ~StackablePictureComponent() override = default;

    //! Set size in function of parent size
    void setNormalisedSize(float width, float height)
    {
      Vector2f pos = denormalise(width, height);
      setSize(pos.x(), pos.y());
    }

    //Loads the image at the given filepath. Will tile if tile is true (retrieves texture as tiling, creates vertices accordingly).
    void setImage(int index, const Path& path, bool tile = false);

    void setOpacity(unsigned char opacity) override;

    void setColor(unsigned int color) override;

    void setColor(int index, unsigned int color);

    void Render(const Transform4x4f& parentTrans) override;

    void Clear() { mImages.clear(); }

    /*!
     * @brief Set image
     * @param index Image index
     * @param visible True (default) to render the image, false to hide it
     */
    StackablePictureComponent& SetVisible(int index, bool visible) { if ((unsigned int)index < mImages.size()) mImages[index].SetVisible(visible); return *this; }

    /*!
     * @brief Set component visibility
     * @param visible True (default) to render the component, false to hide it
     */
    void SetVisible(bool visible) { mVisible = visible; }

    /*!
     * @brief Get component visibility
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
    struct ImageSpec
    {
      StackablePictureComponent& mParent;
      Path mPath;
      Texture mTexture;
      unsigned int mColorShift;
      unsigned char mFadeOpacity;
      ImgProps mProps;
      bool mFading;
      bool mTiled;
      bool mVisible;

      ImageSpec(StackablePictureComponent& parent)
        : mParent(parent)
        , mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(ImgProps::None)
        , mFading(false)
        , mTiled(false)
        , mVisible(true)
      {}

      ImageSpec(StackablePictureComponent& parent, const Path& path, ImgProps props, bool tiled)
        : mParent(parent)
        , mPath(path)
        , mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(props)
        , mFading(false)
        , mTiled(tiled)
        , mVisible(true)
      {
        Load();
      }

      void Set(const Path& path, ImgProps props, bool tiled)
      {
        mTiled = tiled;
        if (mPath != path)
        {
          mPath = path;
          mProps = props;
          Load();
        }
      }

      void SetVisible(bool visible) { mVisible = visible; }

      void Load();

      [[nodiscard]] Colors::ColorRGBA GetColor(unsigned char globalOpacity) const
      {
        unsigned int alpha = ((unsigned int)mFadeOpacity * (unsigned int)globalOpacity) >> 8;
        Colors::ColorRGBA color = (mColorShift & 0xFFFFFF00) | alpha;
        return color;
      }
    };

    std::vector<ImageSpec> mImages;
    ImgProps mProps;
    bool mVisible;

    bool fadeIn(ImageSpec& image);

};

