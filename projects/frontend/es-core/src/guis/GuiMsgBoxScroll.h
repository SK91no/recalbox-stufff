#pragma once

#include <guis/Gui.h>
#include <components/NinePatchComponent.h>
#include <components/ComponentGrid.h>
#include <components/VerticalScrollableContainer.h>
#include "components/MarkdownText.h"
#include "components/TextScrollComponent.h"

class TextComponent;
class ButtonComponent;
class ScrollableContainer;

class GuiMsgBoxScroll : public Gui
{
  public:
    GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
                    const String& name1, const std::function<void()>& func1,
                    const String& name2, const std::function<void()>& func2,
                    const String& name3, const std::function<void()>& func3,
                    ::Alignment align, float size);
    GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
                    const String& name1, const std::function<void()>& func1,
                    const String& name2, const std::function<void()>& func2,
                    const String& name3, const std::function<void()>& func3,
                    ::Alignment align)
      : GuiMsgBoxScroll(window, title, text, name1, func1, name2, func2, name3, func3, align, 0)
    {
    }
    GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
                    const String& name1, const std::function<void()>& func1,
                    const String& name2, const std::function<void()>& func2,
                    const String& name3, const std::function<void()>& func3)
      : GuiMsgBoxScroll(window, title, text, name1, func1, name2, func2, name3, func3, ::Alignment::Center, 0.0f)
    {
    }
    GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
                    const String& name1, const std::function<void()>& func1,
                    const String& name2, const std::function<void()>& func2)
      : GuiMsgBoxScroll(window, title, text, name1, func1, name2, func2, String::Empty, nullptr, ::Alignment::Center, 0.0f)
    {
    }
		GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
										const String& name1, const std::function<void()>& func1,
										const String& name2, const std::function<void()>& func2,
										::Alignment align)
			: GuiMsgBoxScroll(window, title, text, name1, func1, name2, func2, String::Empty, nullptr, align, 0.0f)
    {
    }
		GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
                    const String& name1, const std::function<void()>& func1)
      : GuiMsgBoxScroll(window, title, text, name1, func1, String::Empty, nullptr, String::Empty, nullptr, ::Alignment::Center, 0.0f)
    {
    }
		GuiMsgBoxScroll(WindowManager& window, const String& title, const String& text,
										const String& name1, const std::function<void()>& func1,
										::Alignment align)
			: GuiMsgBoxScroll(window, title, text, name1, func1, String::Empty, nullptr, String::Empty, nullptr, align, 0.0f)
    {
    }

		bool ProcessInput(const InputCompactEvent& event) override;

	  GuiMsgBoxScroll* SetDefaultButton(int index);

		bool CollectHelpItems(Help& help) final;

  private:
		void deleteMeAndCall(const std::function<void()>& func);

		ComponentGrid mGrid;

		std::shared_ptr<TextScrollComponent> mTitle;
	  //std::shared_ptr<VerticalScrollableContainer> mMessageContainer;
		std::shared_ptr<MarkdownText> mMessage;
		std::vector< std::shared_ptr<ButtonComponent> > mButtons;
		std::shared_ptr<ComponentGrid> mButtonGrid;
		std::function<void()> mAcceleratorFunc;

	  int mSpace;
};
