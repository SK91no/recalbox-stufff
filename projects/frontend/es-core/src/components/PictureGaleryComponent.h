//
// Created by bkg2k on 13/10/23.
//
#pragma once

#include "ImgProps.h"
#include <components/base/ThemableComponent.h>
#include "rendering/textures/Texture.h"

class PictureGaleryComponent : public ThemableComponent
{
  public:
    class IChangeNotifications
    {
      public:
        //! Default destructor
        virtual ~IChangeNotifications() = default;

        /*!
         * @brief Allow to notify that the slot has changed (left/right move)
         * @param slot New slot number
         */
        virtual void PictureGalerySlotChanged(int slot) = 0;
    };

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param props Default image properties
     */
    PictureGaleryComponent(WindowManager&window, ImgProps props, IChangeNotifications* callbackInterface);

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PictureGaleryComponent(WindowManager& window, IChangeNotifications* callbackInterface)
      : PictureGaleryComponent(window, ImgProps::None, callbackInterface)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param props Default image properties
     */
    PictureGaleryComponent(WindowManager& window, ImgProps props)
    : PictureGaleryComponent(window, props, nullptr)
    {
    }

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PictureGaleryComponent(WindowManager& window)
      : PictureGaleryComponent(window, ImgProps::None, nullptr)
    {
    }

    //! Destructor
    ~PictureGaleryComponent() override = default;

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

    /*!
     * @brief Set an image path to the given slot. If the image list is not large enough, it's padded with empty path
     * @param slot Slot to assign image to
     * @param path Image path to assign to the given slot
     */
    void SetImage(int slot, const Path& path);

    /*!
     * @brief Set global opacity
     * @param opacity Opacity from 0 to 255
     */
    void setOpacity(unsigned char opacity) override;

    /*!
     * @brief Set color to all slots
     * @param color New color to assign to all slots
     */
    void setColor(unsigned int color) override;

    /*!
     * @brief Set color to the given slot
     * @param slot Slot to assign new clor to
     * @param color New color
     */
    void SetColor(int slot, unsigned int color);

    /*!
     * @brief Clear all images
     */
    void Clear() { mImages.clear(); }

    //! Set background color
    PictureGaleryComponent& SetBackgroundColor(Colors::ColorRGBA color) { mBackgroundColor = color; return *this; }

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
     * @brief Move the cursor, applying the given offset from the current position
     * @param delta Offset to apply tio current position
     */
    void MoveRelative(int delta) { mTargetSlot += delta << 16; }

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
      Path mPath;
      Texture mTexture;
      unsigned int mColorShift;
      unsigned char mFadeOpacity;
      ImgProps mProps;
      bool mFading;

      explicit ImageSpec()
        : mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(ImgProps::None)
        , mFading(false)
      {}

      ImageSpec(const Path& path, int squareSize, ImgProps props)
        : mPath(path)
        , mColorShift(0xFFFFFFFF)
        , mFadeOpacity(0)
        , mProps(props)
        , mFading(false)
      {
        Load(squareSize);
      }

      void Set(const Path& path, int squareSize, ImgProps props)
      {
        if (path != mPath)
        {
          mPath = path;
          mProps = props;
          Load(squareSize);
        }
      }

      void Load(int squareSize);

      [[nodiscard]] Colors::ColorRGBA GetColor(unsigned char globalOpacity, bool fade) const
      {
        unsigned int alpha = ((unsigned int)(fade ? mFadeOpacity : 0xFF) * (unsigned int)globalOpacity) >> 8;
        Colors::ColorRGBA color = (mColorShift & 0xFFFFFF00) | alpha;
        return color;
      }
    };

    //! Callback interface
    IChangeNotifications* mCallbackInterface;

    //! List of image list
    std::vector<ImageSpec> mImages;
    //! Background color
    Colors::ColorRGBA mBackgroundColor;
    //! Selector color
    Colors::ColorRGBA mSelectorColor;
    //! Vertical distance between image in image size ratio
    float mVerticalRatio;
    //! Horizontal distance between image in image size ratio
    float mHorizontalRatio;
    //! Current slot on higher 16 bits
    int mSlot;
    //! Target slot on higher 16 bits
    int mTargetSlot;
    //! Space between item in item ratio
    float mItemGap;
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

