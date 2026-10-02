//
// Created by bkg2k on 11/02/2026.
//
#pragma once
#include "base/Component.h"
#include <mupdf/fitz.h>

#include "rendering/fonts/FontEffect.h"
#include "rendering/textures/Texture.h"

class NewFont;

class PdfExplorerComponent : public Component
{
  public:
    //! Callback interface
    class Interface
    {
      public:
        //! Default destructor
        virtual ~Interface() = default;

        /*!
         * @brief Called when the current page changes
         * @param from Caller
         * @param currentPage New page
         * @param totalPages Total pages
         */
        virtual void PdfPageChanged(PdfExplorerComponent& from, int currentPage, int totalPages) = 0;
    };

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param pdfPath PDF file path
     * @param interface Callback interface
     */
    PdfExplorerComponent(WindowManager& window, NewFont& font, const Path& pdfPath, Interface* interface);

    /*!
     * @brief Constructor
     * @param window Window manager
     * @param interface Callback interface
     */
    explicit PdfExplorerComponent(WindowManager& window, NewFont& font, Interface* interface)
      : PdfExplorerComponent(window, font, Path::Empty, interface)
    {}

    /*!
     * @brief Constructor
     * @param window Window manager
     */
    explicit PdfExplorerComponent(WindowManager& window, NewFont& font)
      : PdfExplorerComponent(window, font, Path::Empty, nullptr)
    {}

    //! Destructor
    ~PdfExplorerComponent() override;

    /*!
     * @brief Close the current document if required and open the new document
     * @param pdfPath New pdf path
     * @return This
     */
    PdfExplorerComponent& SetPdfPath(const Path& pdfPath);

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

    /*!
     * @brief Reinit page display when component size changes
     */
    void onSizeChanged() override { InitializePageDisplay(); }

    //! Get page count
    [[nodiscard]] int PageCount() const { return mPageCount; }

    /*!
     * @brief Load the given document page
     * @param page Page number between 0and page count less one
     */
    void LoadPage(int page) { mCurrentPage = Math::clampi(page, 0, mPageCount - 1); LoadCurrentPage(); }

  private:
    //! Move delay
    static constexpr int sMoveDelay = 250; // ms
    //! Page number display timer
    static constexpr int sPageDisplayTimer = 2000;
    //! Zoom percent display timer
    static constexpr int sZoomPercentTimer = 2000;
    //! Scrollbar timer
    static constexpr int sBarTimer = 1000;

    //! Defgault font effect
    FontEffect mDefaultEffect;

    //! PDF Path
    Path mPath;
    //! Font
    NewFont& mFont;

    //! Callback interface
    Interface* mInterface;

    //! MuPDF Contet
    fz_context* mContext;
    //! MuPDF document
    fz_document* mDocument;

    //! Current page texture
    Texture mPageTexture;

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

    //! Document page available
    int mPageCount;
    //! Current page
    int mCurrentPage;

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

    //! Page number display timer
    int mPageNumberDisplayTimer;
    //! Zoom percent display timer
    int mZoomPercentDisplayTimer;

    //! Valid
    bool mValid;

    /*!
     * @brief Open the pdf and load first page
     * @return true if the document opened successfully, fals eotherwise
     */
    bool OpenDocument();

    //! Close document and deinitialize related member
    void CloseDocument();

    /*!
     * @brief Load current page
     */
    void LoadCurrentPage();

    /*!
     * @brief Initialize page zoom and position
     */
    void InitializePageDisplay();

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
