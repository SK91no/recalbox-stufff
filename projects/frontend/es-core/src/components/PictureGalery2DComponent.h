//
// Created by bkg2k on 13/10/23.
//
#pragma once

#include <memory>
#include "ImgProps.h"
#include <components/base/ThemableComponent.h>
#include "rendering/textures/Texture.h"

class PictureGalery2DComponent : public ThemableComponent
{
  public:
    class IChangeNotifications
    {
      public:
        //! Default destructor
        virtual ~IChangeNotifications() = default;

        /*!
         * @brief Allow to notify that the slot has changed (ledt/right move)
         * @param slot New slot number
         * @param image Current image index in this slot
         */
        virtual void PictureSliderSlotChanged(int slot, int image) = 0;

        /*!
         * @brief Allow to notify that the image in the current slot has changed (up/down move)
         * @param slot Current slot
         * @param image New image index
         */
        virtual void PictureSliderImageChanged(int slot, int image) = 0;
    };

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param imagePath Image path list (allow populating the first slot)
     * @param props Default image properties
     */
    PictureGalery2DComponent(WindowManager&window, const Path::PathList& imagePath, ImgProps props, IChangeNotifications* callbackInterface);

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param props Default image properties
     */
    PictureGalery2DComponent(WindowManager&window, ImgProps props, IChangeNotifications* callbackInterface)
      : PictureGalery2DComponent(window, Path::PathList(), props, callbackInterface)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PictureGalery2DComponent(WindowManager& window, IChangeNotifications* callbackInterface)
      : PictureGalery2DComponent(window, Path::PathList(), ImgProps::None, callbackInterface)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param imagePath Image path list (allow populating the first slot)
     * @param props Default image properties
     */
    PictureGalery2DComponent(WindowManager& window, const Path::PathList& imagePath, ImgProps props)
    : PictureGalery2DComponent(window, imagePath, props, nullptr)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param props Default image properties
     */
    PictureGalery2DComponent(WindowManager&window, ImgProps props)
      : PictureGalery2DComponent(window, Path::PathList(), props, nullptr)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PictureGalery2DComponent(WindowManager& window)
      : PictureGalery2DComponent(window, Path::PathList(), ImgProps::None, nullptr)
    {
    }

    //! Destructor
    ~PictureGalery2DComponent() override = default;

    /*!
     * @brief Notification of an input event
     * @param event Compact event
     * @return Implementation must return true if it consumed the event.
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Called once per frame. Override to implement your own drawings.
     * Call your base::Update() to ensure animation and childrens are updated properly
     * @param deltaTime Elapsed time from the previous frame, in millisecond
     */
    void Update(int deltaTime) override;

    /*!
     * @brief Called once per frame, after Update
     * Implement your own Render() to draw your own compponents or over-drawings
     * First, execute: 	Transform4x4f trans = (parentTrans * getTransform()).round();
	   *                  Renderer::setMatrix(trans);
     * Then draw your components
     * Finally, call your base.Render(trans) to draw animations and childrens
     * @param parentTrans Transformation
     */
    void Render(const Transform4x4f& parentTrans) override;

    //Loads the image at the given filepath. Will tile if tile is true (retrieves texture as tiling, creates vertices accordingly).
    void setImage(int slot, int index, const Path& path, bool tile = false);

    //Loads the image at the given filepath. Will tile if tile is true (retrieves texture as tiling, creates vertices accordingly).
    void setPlaceHolderImage(const Path& path, bool tile = false);

    void setOpacity(unsigned char opacity) override;

    void setColor(unsigned int color) override;

    void setColor(int slot, int index, unsigned int color);

    void Clear() { mImages.clear(); }

    //! Set background color
    PictureGalery2DComponent& SetBackgroundColor(Colors::ColorRGBA color) { mBackgroundColor = color; return *this; }

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

    //! Get real index clamped to image list size
    [[nodiscard]] int Index() const
    {
      int realIndex = (mSlot + 32768) >> 16;
      int imageCount = (int) mImages.size();
      if (!mImages.empty())
      {
        while (realIndex < 0) realIndex += imageCount;
        realIndex %= imageCount;
      }
      return realIndex;
    }

    void SetIndex(int index, bool animate)
    {
      if (mImages.empty()) return;
      while(index < 0) index += (int)mImages.size();
      index %= (int)mImages.size();
      mTargetSlot = index << 16;
      if (!animate) mSlot = mTargetSlot;
    }

  private:
    struct ImageSpec
    {
      PictureGalery2DComponent& mParent;
      Path mPath;
      Texture mTexture;
      unsigned int mColorShift;
      unsigned char mFadeOpacity;
      ImgProps mProps;
      bool mFading;
      bool mTiled;

      explicit ImageSpec(PictureGalery2DComponent& parent)
        : mParent(parent)
        , mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(ImgProps::None)
        , mFading(false)
        , mTiled(false)
      {}

      ImageSpec(PictureGalery2DComponent& parent, const Path& path, ImgProps props, bool tiled)
        : mParent(parent)
        , mPath(path)
        , mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(props)
        , mFading(false)
        , mTiled(tiled)
      {
        Load();
      }

      void Set(const Path& path, ImgProps props, bool tiled)
      {
        mTiled = tiled;
        if (path != mPath)
        {
          mPath = path;
          mProps = props;
          Load();
        }
      }

      void Load();

      [[nodiscard]] Colors::ColorRGBA GetColor(unsigned char globalOpacity, bool fade) const
      {
        unsigned int alpha = ((unsigned int)(fade ? mFadeOpacity : 0xFF) * (unsigned int)globalOpacity) >> 8;
        Colors::ColorRGBA color = (mColorShift & 0xFFFFFF00) | alpha;
        return color;
      }
    };

    class ImageList
    {
      public:
        ImageList()
          : mIndex(0)
          , mTargetIndex()
        {}

        //! Make the index move to target index if required
        void Move(int slot, IChangeNotifications* callbackInterface);

        /*!
         * @brief Notification of an input event
         * @param event Compact event
         * @return Implementation must return true if it consumed the event.
         */
        bool ProcessInput(const InputCompactEvent& event);

        void AddImage(int index, const Path& path, ImgProps props, PictureGalery2DComponent& parent, bool tiled);

        /*
         * Accessors
         */

        //! Get image list
        [[nodiscard]] std::vector<ImageSpec>& Images() { return mImageList; }

        //! Get real index clamped to image list size
        [[nodiscard]] int Index() const
        {
          int realIndex = (mIndex + 32768) >> 16;
          int imageCount = (int) mImageList.size();
          if (!mImageList.empty())
          {
            while (realIndex < 0) realIndex += imageCount;
            realIndex %= imageCount;
          }
          return realIndex;
        }

        //! Get index offset
        [[nodiscard]] int Offset() const { return ((mIndex + 32768) & 0xFFFF) - 32768; }

      private:
        //! Image list
        std::vector<ImageSpec> mImageList;
        //! Current index on higher 16 bits
        int mIndex;
        //! Target index on higher 16 bits
        int mTargetIndex;
    };

    //! Callback interface
    IChangeNotifications* mCallbackInterface;

    //! List of image list
    std::vector<ImageList> mImages;
    //! Placeholder image while loading the real image
    ImageSpec mPlaceHolder;
    //! Background color
    Colors::ColorRGBA mBackgroundColor;
    //! Vertical distance between image in image size ratio
    float mVerticalRatio;
    //! Horizontal distance between image in image size ratio
    float mHorizontalRatio;
    //! Current slot on higher 16 bits
    int mSlot;
    //! Target slot on higher 16 bits
    int mTargetSlot;
    //! Image properties
    ImgProps mProps;
    //! Visible or not ?
    bool mVisible;

    /*!
     * @brief Check if the image is ready
     * @param image Image to check
     * @return True if the image is not ready yet, false otherwise
     */
    bool fadeIn(ImageSpec& image);

    //! Get real index clamped to image list size
    [[nodiscard]] int Slot(int offset) const
    {
      int realIndex = (mSlot + 32768) >> 16;
      int imageCount = (int) mImages.size();
      if (!mImages.empty())
      {
        realIndex += offset;
        while (realIndex < 0) realIndex += imageCount;
        realIndex %= imageCount;
      }
      return realIndex;
    }

    //! Get index offset
    [[nodiscard]] int Offset() const { return ((mSlot + 32768) & 0xFFFF) - 32768; }

    //! Make the slot move to target index if required
    void Move();
};

