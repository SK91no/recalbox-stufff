//
// Created by bkg2k on 11/02/2026.
//

#include "PdfExplorerComponent.h"

#include "Renderer.h"
#include "help/Help.h"
#include "input/InputCompactEvent.h"
#include "rendering/fonts/Font.h"
#include "rendering/textures/TextureManager.h"
#include "utils/locale/LocaleHelper.h"

PdfExplorerComponent::PdfExplorerComponent(WindowManager& window, NewFont& font, const Path& pdfPath, PdfExplorerComponent::Interface* interface)
  : Component(window)
  , mPath(pdfPath)
  , mFont(font)
  , mInterface(interface)
  , mContext(nullptr)
  , mDocument(nullptr)
  , mMaxZoom(0)
  , mMinZoom(0)
  , mStepZoom(0)
  , mCurrentZoom(0)
  , mTargetZoom(0)
  , mPageCount(0)
  , mCurrentPage(0)
  , mHDirection(0)
  , mVDirection(0)
  , mMoveDelay(0)
  , mHBarTimer(0)
  , mVBarTimer(0)
  , mScrollbarThickness(Renderer::Instance().DisplayWidthAsInt() > 1024 ? 6 : (Renderer::Instance().DisplayWidthAsInt() > 480 ? 4 : 2))
  , mPageNumberDisplayTimer(0)
  , mZoomPercentDisplayTimer(0)
  , mValid(false)
{
  if (pdfPath.Exists())
    OpenDocument();
}

PdfExplorerComponent::~PdfExplorerComponent()
{
  CloseDocument();
}

PdfExplorerComponent& PdfExplorerComponent::SetPdfPath(const Path& pdfPath)
{
  CloseDocument();
  mPath = pdfPath;
  OpenDocument();
  return *this;
}

bool PdfExplorerComponent::ProcessInput(const InputCompactEvent& event)
{
  if (mThemeDisabled) return false;

  // Sustained moves
  if (event.UpPressed()) { mMoveDelay = sMoveDelay; mVDirection = 1; return true; }
  if (event.DownPressed()) { mMoveDelay = sMoveDelay; mVDirection = -1; return true; }
  if (event.LeftPressed()) { mMoveDelay = sMoveDelay; mHDirection = 1; return true; }
  if (event.RightPressed()) { mMoveDelay = sMoveDelay; mHDirection = -1; return true; }
  if (event.J1UpPressed()) { mMoveDelay = sMoveDelay; mVDirection = 4; return true; }
  if (event.J1DownPressed()) { mMoveDelay = sMoveDelay; mVDirection = -4; return true; }
  if (event.J1LeftPressed()) { mMoveDelay = sMoveDelay; mHDirection = 4; return true; }
  if (event.J1RightPressed()) { mMoveDelay = sMoveDelay; mHDirection = -4; return true; }
  // Single step moves
  if (event.UpReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(0, 1, 0); return true; }
  if (event.DownReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(0, -1, 0); return true; }
  if (event.LeftReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(1, 0, 0); return true; }
  if (event.RightReleased() && mMoveDelay != 0) { mVDirection = mHDirection = mMoveDelay = 0; MovePage(-1, 0, 0); return true; }
  // Stop moves
  if (event.UpReleased() || event.DownReleased()) { mVDirection = 0; return true; }
  if (event.LeftReleased() || event.RightReleased()) { mHDirection = 0; return true; }

  // Zoom in/out
  if (event.NorthReleased()) { ZoomPage(true); return true; }
  if (event.WestReleased()) { ZoomPage(false); return true; }

  // Page change
  if (event.L1Released()) { if (--mCurrentPage < 0) mCurrentPage = mPageCount - 1; LoadCurrentPage(); return true; }
  if (event.R1Released()) { if (++mCurrentPage >= mPageCount) mCurrentPage = 0; LoadCurrentPage(); return true; }

  return Component::ProcessInput(event);
}

void PdfExplorerComponent::Update(int deltaTime)
{
  if (mThemeDisabled) return;

  // Scrollbar
  if (mHBarTimer > 0) if (mHBarTimer -= deltaTime; mHBarTimer < 0) mHBarTimer = 0;
  if (mVBarTimer > 0) if (mVBarTimer -= deltaTime; mVBarTimer < 0) mVBarTimer = 0;

  // Sustained move
  if (mMoveDelay > 0) { if (mMoveDelay -= deltaTime; mMoveDelay < 0) mMoveDelay = 0; return; }
  if (mVDirection != 0 || mHDirection != 0) MovePage(mHDirection, mVDirection, deltaTime);

  // OSD
  if (mPageNumberDisplayTimer > 0)
    if (mPageNumberDisplayTimer -= deltaTime; mPageNumberDisplayTimer < 0) mPageNumberDisplayTimer = 0;
  if (mZoomPercentDisplayTimer > 0)
    if (mZoomPercentDisplayTimer -= deltaTime; mZoomPercentDisplayTimer < 0) mZoomPercentDisplayTimer = 0;

  // Zoom animation
  AnimateZoom();
}

void PdfExplorerComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  Renderer::Instance().Clip(trans, mSize);

  // Render PDF image
  mPageTexture.Render(Math::roundi(mPageArea.Left()), Math::roundi(mPageArea.Top()),
                      Math::roundi(mPageArea.Width()), Math::roundi(mPageArea.Height()),
                      false, true, true, false, Colors::Common::White);
  // Render Page number
  if (mPageNumberDisplayTimer > 0 || mZoomPercentDisplayTimer > 0)
  {
    Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
    if (mPageNumberDisplayTimer > 0)
    {
      if (mZoomPercentDisplayTimer > 0) displayArea.MoveBottom((float)-mFont.Height());
      String page = (_F(_("Page {0}/{1}")) / (mCurrentPage + 1) / mPageCount)();
      int alpha = Math::clampi(mPageNumberDisplayTimer, 0, 255);
      unsigned int white = 0xFFFFFF00 | alpha;
      mFont.DrawText(page, displayArea, -1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, -1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, 1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0,0 , alpha, Alignment::Center, mDefaultEffect);
    }
    if (mZoomPercentDisplayTimer > 0)
    {
      if (mPageNumberDisplayTimer > 0) displayArea.Move(0, (float)mFont.Height());
      String page = (_F(_("Zoom {0}%")) / (int)(mTargetZoom / mMaxZoom * 200.f))();
      int alpha = Math::clampi(mZoomPercentDisplayTimer, 0, 255);
      unsigned int white = 0xFFFFFF00 | alpha;
      mFont.DrawText(page, displayArea, -1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 1, 0, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, -1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0, 1, white, Alignment::Center, mDefaultEffect);
      mFont.DrawText(page, displayArea, 0,0 , alpha, Alignment::Center, mDefaultEffect);
    }
  }

  // Draw vertical bar
  if (mVBarTimer > 0)
    if (float ratio = mSize.y() / mPageArea.Height(); ratio < 1.f)
    {
      int barHeight = Math::roundi(ratio * mSize.y());
      Renderer& renderer = Renderer::Instance();
      if ((barHeight < renderer.DisplayHeightAsInt() / 40) && (mSize.y() >= renderer.DisplayHeightAsFloat() / 20.f)) barHeight = renderer.DisplayHeightAsInt() / 40;
      int yOffset = (int)((mSize.y() - (float)barHeight - 4.f) * (-mPageArea.Top() / (mPageArea.Height() - mSize.y())));
      int alpha = Math::clampi(mVBarTimer / 2, 0, 255);
      Renderer::DrawRectangle((int) mSize.x() - 4 - mScrollbarThickness, 1 + yOffset, mScrollbarThickness + 2, barHeight + 2, 0xFFFFFF00 | alpha);
      Renderer::DrawRectangle((int) mSize.x() - 3 - mScrollbarThickness, 2 + yOffset, mScrollbarThickness, barHeight, alpha);
    }

  // Draw horizontal bar
  if (mHBarTimer > 0)
    if (float ratio = mSize.x() / mPageArea.Width(); ratio < 1.f)
    {
      int barWidth = Math::roundi(ratio * mSize.x());
      Renderer& renderer = Renderer::Instance();
      if ((barWidth < renderer.DisplayHeightAsInt() / 40) && (mSize.x() >= renderer.DisplayHeightAsFloat() / 20.f)) barWidth = renderer.DisplayHeightAsInt() / 40;
      int xOffset = (int)((mSize.x() - (float)barWidth - 4.f) * (-mPageArea.Left() / (mPageArea.Width() - mSize.x())));
      int alpha = Math::clampi(mVBarTimer / 2, 0, 255);
      Renderer::DrawRectangle(1 + xOffset, (int) mSize.y() - 4 - mScrollbarThickness, barWidth + 2, mScrollbarThickness + 2, 0xFFFFFF00 | alpha);
      Renderer::DrawRectangle(2 + xOffset, (int) mSize.y() - 3 - mScrollbarThickness, barWidth, mScrollbarThickness, alpha);
    }

  Renderer::Instance().Unclip();
}

bool PdfExplorerComponent::CollectHelpItems(Help& help)
{
  help.Set(HelpType::AllDirections, _("MOVE"))
      .Set(HelpType::LR, _("PAGE +/-"))
      .Set(HelpType::X, _("ZOOM +"))
      .Set(HelpType::Y, _("ZOOM -"));
  return true;
}

bool PdfExplorerComponent::OpenDocument()
{
  // Create context. FZ_STORE_DEFAULT (256 MB) matches mutool; unlimited is uncapped and
  // can push low-RAM targets to swap without offering any rendering benefit.
  mContext = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
  if (mContext == nullptr) return false;
  fz_register_document_handlers(mContext);

  // Match mutool's context configuration: default 8-bit AA for text/graphics. Without these
  // calls MuPDF uses compile-time defaults which may differ depending on AA_BITS define.
  fz_set_text_aa_level(mContext, 8);
  fz_set_graphics_aa_level(mContext, 8);

  // CRITICAL: never `return`, `break`, `continue` or `goto` out of an fz_try block.
  // Doing so leaves an unpopped exception frame on MuPDF's internal stack, corrupting
  // every subsequent fz_try in the same context. Symptoms vary by platform — on ARM with
  // a different stack layout than x86, this manifests as silently-aborted page renders
  // producing fully-black pixmaps. Use a local success flag and exit naturally instead.
  bool ok = false;
  fz_try(mContext)
  {
    mDocument = fz_open_document(mContext, mPath.ToChars());
    mPageCount = fz_count_pages(mContext, mDocument);
    mCurrentPage = 0;
    ok = true;
  }
  fz_catch(mContext)
  {
    { LOG(LogError) << "[PdfExplorerComponent] Error code " << fz_caught(mContext) << " : " << fz_caught_message(mContext); }
    CloseDocument();
  }
  return ok;
}

void PdfExplorerComponent::CloseDocument()
{
  if (mDocument != nullptr) fz_drop_document(mContext, mDocument);
  if (mContext != nullptr) fz_drop_context(mContext);
  mDocument = nullptr;
  mContext = nullptr;
}

void PdfExplorerComponent::LoadCurrentPage()
{
  // Unsized component
  if (mSize.isZero() || mPageCount < 1) return;

  fz_page* page = nullptr;
  fz_pixmap* pix = nullptr;
  fz_try(mContext)
  {
    page = fz_load_page(mContext, mDocument, mCurrentPage);
    fz_rect bounds = fz_bound_page(mContext, page);

    // Rendering zoom - Maximum 2 x display width
    float zoom = CalculateRenderingZoom(bounds.x1 - bounds.x0, bounds.y1 - bounds.y0);
    fz_matrix transform = fz_scale(zoom, zoom);

    // Manual render mirroring mudraw.c (mutool draw). The MuPDF high-level helper
    // fz_new_pixmap_from_page bakes the CTM into the device (identity to run_page);
    // mudraw does the opposite (identity to device, CTM to run_page). Both are
    // mathematically equivalent but trigger different internal code paths. The helper's
    // path appears to be broken on ARM for some PDFs (black pixmaps), the mudraw path
    // is known to work (mutool renders fine on the same Pi 5).
    fz_rect transformedBounds = fz_transform_rect(bounds, transform);
    fz_irect bbox = fz_round_rect(transformedBounds);
    pix = fz_new_pixmap_with_bbox(mContext, fz_device_rgb(mContext), bbox, nullptr, 0);
    fz_clear_pixmap_with_value(mContext, pix, 0xFF); // white paper background

    fz_device* dev = fz_new_draw_device(mContext, fz_identity, pix);
    fz_run_page(mContext, page, dev, transform, nullptr);
    fz_close_device(mContext, dev);
    fz_drop_device(mContext, dev);

    int width = fz_pixmap_width(mContext, pix);
    int height = fz_pixmap_height(mContext, pix);
    int stride = fz_pixmap_stride(mContext, pix);
    const unsigned char* samples = fz_pixmap_samples(mContext, pix);

    // Pack MuPDF's RGB pixmap (3 bytes/pixel) into our RGBA buffer (uint32/pixel) with
    // forced opaque alpha. Single 32-bit store per pixel is significantly faster on ARM
    // than four byte stores. Row-by-row copy also handles MuPDF stride padding (NEON
    // alignment may pad on ARM, although uncommon for alpha=0 pixmaps).
    // Little-endian layout: byte order in memory is [R, G, B, A] → uint32 = A<<24|B<<16|G<<8|R.
    Array<unsigned int> rgba;
    rgba.ExpandTo(width * height);
    unsigned int* dst = rgba.BufferReadWrite();
    for (int y = 0; y < height; ++y)
    {
      const unsigned char* src = samples + (size_t)y * (size_t)stride;
      for (int x = 0; x < width; ++x, src += 3)
        *dst++ = 0xFF000000u | ((unsigned int)src[2] << 16) | ((unsigned int)src[1] << 8) | (unsigned int)src[0];
    }

    mPageTexture = TextureManager::Instance().Create(Path::Empty, width, height, TextureHolder::Properties::Free | TextureHolder::Properties::NoCache);
    if (!mPageTexture.UploadRGBA(width, height, rgba.BufferReadWrite()))
    { LOG(LogError) << "[PdfExplorerComponent] Cannot upload page texture of " << width << 'x' << height << " pixels"; }

    fz_drop_pixmap(mContext, pix); pix = nullptr;
    fz_drop_page(mContext, page); page = nullptr;

    InitializePageDisplay();
    if (mInterface != nullptr)
      mInterface->PdfPageChanged(*this, mCurrentPage + 1, mPageCount);
    mPageNumberDisplayTimer = sPageDisplayTimer;
  }
  fz_catch(mContext)
  {
    if (pix != nullptr) fz_drop_pixmap(mContext, pix);
    if (page != nullptr) fz_drop_page(mContext, page);
    { LOG(LogError) << "[PdfExplorerComponent] Error code " << fz_caught(mContext) << " : " << fz_caught_message(mContext); }
    CloseDocument();
  }
}

void PdfExplorerComponent::InitializePageDisplay()
{
  if (mPageTexture.Valid())
  {
    Vector2i textureSize = mPageTexture.Size();
    // Initialize zoom
    mCurrentZoom = mTargetZoom = mSize.x() / (float)textureSize.x();
    mMaxZoom = mCurrentZoom * 2.f;
    mMinZoom = mCurrentZoom / 2.f;
    mStepZoom = mCurrentZoom / 4.f;
    // Reset position
    mPageArea.Set(0, 0, mCurrentZoom * (float)textureSize.x(), mCurrentZoom * (float)textureSize.y()).Round();
  }
}

float PdfExplorerComponent::CalculateRenderingZoom(float originalWidth, float originalHeight)
{
  // Get both ratio
  float xRatio = mSize.x() / originalWidth;
  float yRatio = mSize.y() / originalHeight;

  // Get the lower ratio and compute the rendering zoom
  if (xRatio < yRatio) return (mSize.x() * 2.f) / originalWidth;
  return (mSize.y() * 2.f) / originalHeight;
}

void PdfExplorerComponent::MovePage(int xDir, int yDir, int elapsedTime)
{
  float yMove = 0;
  float xMove = 0;
  if (elapsedTime == 0) elapsedTime = 20;
  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
  displayArea.Round();

  // Move ?
  if ((yDir > 0 && mPageArea.Top() < displayArea.Top()) ||
      (yDir < 0 && mPageArea.Bottom() > displayArea.Bottom())) yMove = (float)(elapsedTime * yDir);
  if ((xDir > 0 && mPageArea.Left() < displayArea.Left()) ||
      (xDir < 0 && mPageArea.Right() > displayArea.Right())) xMove = (float)(elapsedTime * xDir);

  // Move !
  if (xMove != 0 || yMove != 0)
  {
    xMove = Math::round((xMove / 20.f) * (Renderer::Instance().DisplayHeightAsFloat() / 50.f));
    yMove = Math::round((yMove / 20.f) * (Renderer::Instance().DisplayHeightAsFloat() / 50.f));
    mPageArea.Move(xMove, yMove);
  }

  AdjustBounds();
}

void PdfExplorerComponent::ZoomPage(bool zoomIn)
{
  if (zoomIn) { if (mTargetZoom >= mMaxZoom) return; }
  else if (mTargetZoom <= mMinZoom) return;

  mTargetZoom += zoomIn ? mStepZoom : -mStepZoom;
  mTargetZoom = Math::clamp(mTargetZoom, mMinZoom, mMaxZoom);

  mZoomPercentDisplayTimer = sZoomPercentTimer;
}

void PdfExplorerComponent::AdjustBounds()
{
  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());

  // Adjust page bounds
  if (mPageArea.Width() > Math::round(displayArea.Width()))
  {
    if (mPageArea.Right() < displayArea.Right()) mPageArea.Move(displayArea.Right() - mPageArea.Right(), 0);
    if (mPageArea.Left() > displayArea.Left()) mPageArea.Move(displayArea.Left() - mPageArea.Left(), 0);
    mHBarTimer = sBarTimer;
  }
  else mPageArea.SetLeft((displayArea.Width() - mPageArea.Width()) / 2);

  if (mPageArea.Height() > Math::round(displayArea.Height()))
  {
    if (mPageArea.Bottom() < displayArea.Bottom()) mPageArea.Move(0, displayArea.Bottom() - mPageArea.Bottom());
    if (mPageArea.Top() > displayArea.Top()) mPageArea.Move(0, displayArea.Top() - mPageArea.Top());
    mVBarTimer = sBarTimer;
  }
  else mPageArea.SetTop((displayArea.Height() - mPageArea.Height()) / 2);
}

void PdfExplorerComponent::AnimateZoom()
{
  if (mTargetZoom == mCurrentZoom) return;

  // Zoom animation
  float height = (float)mPageTexture.Height();
  float minStep = Math::abs((height * mTargetZoom) - (height * mCurrentZoom));
  if (minStep <= 2.f) mCurrentZoom = mTargetZoom;
  else mCurrentZoom = (mCurrentZoom + mTargetZoom) / 2.f;

  Rectangle displayArea(0.f, 0.f, mSize.x(), mSize.y());
  float xRatio = (displayArea.XCenter() - mPageArea.Left()) / mPageArea.Width();
  float yRatio = (displayArea.YCenter() - mPageArea.Top()) / mPageArea.Height();

  Vector2i size = mPageTexture.Size();
  mPageArea.Set(displayArea.XCenter() - ((float)size.x() * xRatio), displayArea.YCenter() - ((float)size.y() * yRatio), (float)size.x(), (float)size.y())
           .ZoomRelative(mCurrentZoom, xRatio, yRatio);

  AdjustBounds();
}
