//
// Created by bkg2k on 16/02/2026.
//
#pragma once
#include "base/Component.h"
#include "rendering/fonts/FontEffect.h"
#include "rendering/textures/Texture.h"
#include "rendering/fonts/Font.h"

class PictureExplorerComponent : public Component
{
  public:
    /*!
     * @brief Constructor
     * @param window Window manager
     * @param font OSD font
     * @param picturePath PDF file path
     */
    PictureExplorerComponent(WindowManager& window, NewFont& font, const Path& picturePath);

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param font OSD font
     */
    explicit PictureExplorerComponent(WindowManager& window, NewFont& font)
      : PictureExplorerComponent(window, font, Path::Empty)
    {}

    /*!
     * @brief Close the current document if required and open the new document
     * @param pdfPath New pdf path
     * @return This
     */
    PictureExplorerComponent& SetPicturePath(const Path& pdfPath);

    /*!
     * @brief Manage inpout event to move/zoom the current pdf page
     * @param event Input event
     * @return True if the method consumed the event
     */
    bool ProcessInput(const InputCompactEvent& event) override;

    /*!
     * @brief Update move, if required
     * @param deltaTime Elapsed time since last update
     */
    void Update(int deltaTime) override;

    /*!
     * @brief Override rendering to draw custom background
     * @param parentTrans parent transformation
     */
    void Render(const Transform4x4f& parentTrans) override;

    /*!
      * @brief Set help prompts
      * @param help Help structure to fill in
      * @return True
      */
    bool CollectHelpItems(Help& help) override;

  private:
    //! Move delay
    static constexpr int sMoveDelay = 250; // ms
    //! Zoom percent display timer
    static constexpr int sZoomPercentTimer = 2000;
    //! Scrollbar timer
    static constexpr int sBarTimer = 1000;

    //! Default font effect
    FontEffect mDefaultEffect;

    //! PDF Path
    Path mPath;
    //! Font
    NewFont& mFont;

    //! Current page texture
    Texture mPageTexture;

    //! Image initialized
    bool mImageInitialized;
    //! Global Alpha
    int mGlobalAlpha;

    //! Max zoom
    float mMaxZoom;
    //! Min zoom
    float mMinZoom;
    //! Step zoom
    float mStepZoom;
    //! Current zoom
    float mCurrentZoom;
    //! Target zoom (Current zoom move to this value to animate the zoom
    float mTargetZoom;
    //! Current displayed page area, at the current zoom
    Rectangle mPageArea;

    //! Horizontal move direction
    int mHDirection;
    //! Vertical move direction
    int mVDirection;
    //! Move timer
    int mMoveDelay;

    //! Horizontal scrollbar timer
    int mHBarTimer;
    //! Vertical scrollbar timer
    int mVBarTimer;
    //! Scroll bar thickness
    int mScrollbarThickness;

    //! Zoom percent display timer
    int mZoomPercentDisplayTimer;

    /*!
     * @brief Load current page
     */
    void LoadPicture();

    /*!
     * @brief Initialize zoom & position
     */
    bool InitializeDisplay();

    /*!
     * @brief Calculate the rendering zoom, regarding the original size and screen size limitations
     * @param originalWidth Original page width
     * @param originalHeight Original page Height
     * @return Zoom factor used to render the page to bitmap
     */
    float CalculateRenderingZoom(float originalWidth, float originalHeight);

    /*!
     * @brief Move page one step in the given directions
     * @param xDir Horizontal direction
     * @param yDir Vertical direction
     * @param elapsedTime Elapsed millisecond since last call
     */
    void MovePage(int xDir, int yDir, int elapsedTime);

    /*!
     * @brief Zoom one step in or out
     * @param zoomIn True to zoome in, false to zoom out
     */
    void ZoomPage(bool zoomIn);

    /*!
     * @brief Adjust page bounds into the display area
     */
    void AdjustBounds();

    /*!
     * @brief Make Current zoom move to target zoom
     */
    void AnimateZoom();
};
