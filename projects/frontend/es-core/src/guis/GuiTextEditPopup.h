#include <guis/Gui.h>
#include <components/ComponentGrid.h>
#include <components/TextEditComponent.h>
#include <components/TextComponent.h>

class GuiTextEditPopup : public Gui
{
public:
	GuiTextEditPopup(WindowManager&window, const String& title, const String& initValue,
                   const std::function<void(const String&)>& okCallback, bool multiLine,
                   const String& acceptBtnText);
  GuiTextEditPopup(WindowManager&window, const String& title, const String& initValue,
                   const std::function<void(const String&)>& okCallback, bool multiLine)
    : GuiTextEditPopup(window, title, initValue, okCallback, multiLine, "OK")
  {
  }

	bool ProcessInput(const InputCompactEvent& event) override;
	void onSizeChanged() override;
	bool CollectHelpItems(Help& help) override;

private:
	ComponentGrid mGrid;

	std::shared_ptr<TextComponent> mTitle;
	std::shared_ptr<TextEditComponent> mText;
	std::shared_ptr<ComponentGrid> mButtonGrid;

	bool mMultiLine;
};
