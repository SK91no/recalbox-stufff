//
// Created by bkg2k on 20/11/2019.
//

#include <WindowManager.h>
#include <guis/Gui.h>

#include "rendering/textures/TextureManager.h"
#include "themes/ThemeManager.h"

Gui::Gui(WindowManager& window)
  : Component(window)
  , mTheme(ThemeManager::Instance().Menu())
  , mFramePath(mTheme.Background().path)
  , mFrame(TextureManager::Instance().Create(mFramePath, 0, 0, TextureHolder::Properties::None))
  , mGuiClosePending(false)
  , mRenderer(Renderer::Instance())
{
}

void Gui::CenterGui()
{
  setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2,
              (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);
}

bool Gui::AmIOnTopOfScreen() const
{
  return mWindow.AmIOnTopOfScreen(this);
}

void Gui::Render(const Transform4x4f& parentTrans)
{
  Transform4x4f trans = parentTrans * getTransform();
  Renderer::SetMatrix(trans);

  // Auto sync with theme - bad anyway...
  if (mFramePath != mTheme.Background().path)
  {
    mFramePath = mTheme.Background().path;
    mFrame.Set(TextureManager::Instance().Create(mFramePath, 0, 0, TextureHolder::Properties::None));
  }

  // Draw frame
  int fw = 0; //mFrame.ChunkWidth();
  int fh = 0; //mFrame.ChunkHeight();
  //if (mRenderer.Is240p()) { fw >>= 1; fh >>= 1; }
  Rectangle frame(-fw, -fh, (int)getWidth() + (fw * 2), (int)getHeight() + (fh * 2));
  mFrame.Render(frame,  mTheme.Background().color);

  // Children
  Component::Render(parentTrans);
}

