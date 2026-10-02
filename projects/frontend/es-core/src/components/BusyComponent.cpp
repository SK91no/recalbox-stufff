#include "BusyComponent.h"

#include "components/AnimatedImageComponent.h"
#include "components/TextComponent.h"
#include "Renderer.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>

// animation definition
static AnimationFrame BUSY_ANIMATION_FRAMES[]
{
  { .path=":/busy_0.png", .time=300 },
  { .path=":/busy_1.png", .time=300 },
  { .path=":/busy_2.png", .time=300 },
  { .path=":/busy_3.png", .time=300 },
};
const AnimationDef BUSY_ANIMATION_DEF = { BUSY_ANIMATION_FRAMES, 4, true };

BusyComponent::BusyComponent(WindowManager&window)
  : Component(window)
  , mFrame(window, Path(":/frame.png"))
  , mGrid(window,Vector2i(5, 6))
  , mHighestWidth(0)
  , mThreadMessagechanged(false)
  , mEnabled(true)
  , mKeepHighestWidth(false)
  , mShowProgressBar(false)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

  mFrame.setImagePath(menuTheme.Background().path);
  mFrame.setCenterColor(menuTheme.Background().color);
  mFrame.setEdgeColor(menuTheme.Background().color);

  mAnimation = std::make_shared<AnimatedImageComponent>(mWindow);
  mAnimation->load(&BUSY_ANIMATION_DEF);
  mText = std::make_shared<TextComponent>(mWindow, _("WORKING..."), menuTheme.Text().font, menuTheme.Text().color);
  mSubText = std::make_shared<TextComponent>(mWindow, "", menuTheme.Text().font, menuTheme.Text().color);
  mProgress = std::make_shared<ProgressBarComponent>(window, 100);

  // col 0 = animation, col 1 = spacer, col 2 = text
  mGrid.setEntry(mAnimation, Vector2i(1, 1), false, true);
  mGrid.setEntry(mText, Vector2i(3, 1), false, true);
  mGrid.setEntry(mSubText, Vector2i(1, 3), false, true, Vector2i(3, 1));

  addChild(&mFrame);
  addChild(&mGrid);
}

void BusyComponent::SetText(const String& txt, bool keepHighestWidth)
{
  if (mMutex.Lock())
  {
    mKeepHighestWidth = keepHighestWidth;
    mThreadMessage = txt;
    mThreadMessagechanged = true;
    mMutex.UnLock();
  }
}

void BusyComponent::SetSecondaryText(const String& txt)
{
  if (mMutex.Lock())
  {
    mThreadMessage2 = txt;
    mThreadMessagechanged = true;
    mMutex.UnLock();
  }
}

String BusyComponent::getText()
{
  Mutex::AutoLock lock(mMutex);
  return mText->getValue();
}

void BusyComponent::Render(const Transform4x4f& parentTrans)
{
  if (!mEnabled) return;

  if (mMutex.Lock())
  {
    if(mThreadMessagechanged)
    {
      mThreadMessagechanged = false;
      mText->setText(mThreadMessage);
      mSubText->setText(mThreadMessage2);
      onSizeChanged();
    }
    mMutex.UnLock();
  }
  Component::Render(parentTrans);
}

void BusyComponent::onSizeChanged()
{
  mGrid.setSize(mSize);

  if(mSize.x() == 0 || mSize.y() == 0) return;

  bool HasSub = !mSubText->getValue().empty();
  const float middleSpacerWidth = 0.01f * Renderer::Instance().DisplayWidthAsFloat();
  const float textHeight = mText->getFont()->Height();
  mText->setSize(0, textHeight);
  if (HasSub) mSubText->setSize(0, textHeight);
  float textWidth = Math::max(mText->getFont()->TextWidth(mText->getValue()),
                              mSubText->getFont()->TextWidth(mSubText->getValue()));
  textWidth = Math::min(textWidth, Renderer::Instance().DisplayWidthAsFloat() * 0.8f) + 4;

  mGrid.setColWidthPerc(1, textHeight / mSize.x()); // animation is square
  mGrid.setColWidthPerc(2, middleSpacerWidth / mSize.x());
  if (mKeepHighestWidth)
  {
    if (textWidth > mHighestWidth) mHighestWidth = textWidth;
    mGrid.setColWidthPerc(3, mHighestWidth / mSize.x() <= 1 ? mHighestWidth / mSize.x() : 1);
  }
  else mGrid.setColWidthPerc(3, textWidth / mSize.x() <= 1 ? textWidth / mSize.x() : 1);

  mGrid.setRowHeightPerc(1, textHeight / mSize.y() <= 1 ? textHeight / mSize.y() : 1);
  mGrid.setRowHeightPerc(2, (textHeight / 1.5f) / mSize.y());
  if (HasSub) mGrid.setRowHeightPerc(3, textHeight / mSize.y() <= 1 ? textHeight / mSize.y() : 1);
  else mGrid.setRowHeightPerc(3, 0.0001);
  if (mShowProgressBar) mGrid.setRowHeightPerc(4, 0.02f);
  else mGrid.setRowHeightPerc(4, 0.0001);

  mFrame.fitTo(Vector2f(mGrid.getColWidth(1) + mGrid.getColWidth(2) + mGrid.getColWidth(3), mGrid.getRowHeight(1, 4)), mAnimation->getPosition(), Vector2f(0, 0));
}

void BusyComponent::SetProgress(int progress)
{
  mProgress->setCurrentValue(progress);
  if (!mShowProgressBar)
  {
    mProgress->setMaxValue(100);
    mGrid.setEntry(mProgress, Vector2i(1, 4), false, true, Vector2i(3, 1));
    mShowProgressBar = true;
  }
}

