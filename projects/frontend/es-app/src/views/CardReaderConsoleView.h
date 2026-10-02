//
// Created by digi on 12/22/25.
//

#pragma once

#include <WindowManager.h>
#include "components/VideoComponent.h"

class CardReaderConsoleView : public Gui
{
  public:
    explicit CardReaderConsoleView(WindowManager& window);
    void Render(const Transform4x4f& parentTrans) final;
    bool ProcessInput(const InputCompactEvent& event) override;
    void Reset();

  private:
    VideoComponent mVideo;
    bool started;
    DateTime mStartPressed;
    bool exit();
};
