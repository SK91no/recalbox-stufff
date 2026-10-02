#include <guis/GuiMsgBoxScroll.h>
#include <components/TextComponent.h>
#include <components/ButtonComponent.h>
#include <components/MenuComponent.h> // for makeButtonGrid

//#define HORIZONTAL_PADDING_PX 20

GuiMsgBoxScroll::GuiMsgBoxScroll(WindowManager& window,
                                 const String& title, const String& text,
                                 const String& name1, const std::function<void()>& func1,
                                 const String& name2, const std::function<void()>& func2,
                                 const String& name3, const std::function<void()>& func3,
                                 ::Alignment align, float height)
  : Gui(window)
  , mGrid(window, Vector2i(3, 3))
  , mSpace(Renderer::Instance().DisplayHeightAsInt() / 40)
{
  if (mSpace > 20) mSpace = 20;
  if (mSpace < 6) mSpace = 6;
	(void)height;

	float maxWidth = Renderer::Instance().DisplayWidthAsFloat() * 0.8f; // max width
	float minWidth = Renderer::Instance().DisplayWidthAsFloat() * 0.4f; // minimum width
  float width = maxWidth;

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

	mTitle = std::make_shared<TextScrollComponent>(mWindow, title, menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);

  //new MarkdownText(mWindow, text, menuTheme.SmallText().color, Alignment::TopLeft);
  //mMessageContainer = std::make_shared<VerticalScrollableContainer>(mWindow);
	mMessage = std::make_shared<MarkdownText>(mWindow, text, menuTheme.SmallText().color, AlignmentReplaceVertical(align, VerticalAlignment::Top), true);
  mMessage->setSize(width, 0);
  //mMessageContainer->addChild(mMessage.get());

	// create the buttons
	mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name1, name1, std::bind(&GuiMsgBoxScroll::deleteMeAndCall, this, func1)));
	if(!name2.empty()) mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name2, name3, std::bind(&GuiMsgBoxScroll::deleteMeAndCall, this, func2)));
	if(!name3.empty()) mButtons.push_back(std::make_shared<ButtonComponent>(mWindow, name3, name3, std::bind(&GuiMsgBoxScroll::deleteMeAndCall, this, func3)));

	// put the buttons into a ComponentGrid
	mButtonGrid = MenuComponent::MakeButtonGrid(mWindow, mButtons);

  width = Math::max(mButtonGrid->getSize().x(), (float)mMessage->Width());
  width = Math::max(width, Math::min((float)menuTheme.Title().font->TextWidth(title), maxWidth));
  width = Math::max(width, minWidth);
  mMessage->setSize(width, 0); // Force recalculation of message height

  const float msgHeight = Math::min(Renderer::Instance().DisplayHeightAsFloat() * 0.5f, (float)mMessage->Height());

  mGrid.setEntry(mTitle, Vector2i(1, 0), false, true);
  mGrid.setEntry(mMessage, Vector2i(1, 1), false, true);
  mGrid.setEntry(mButtonGrid, Vector2i(1, 2), true, false, Vector2i(1, 1));

  mGrid.setSize(width + ((float)mSpace * 2), (mButtonGrid->getSize().y() * 1.2f) + (msgHeight + mTitle->getSize().y() * 1.5f));
  mGrid.setRowHeightPerc(0, (float)mTitle->getFont()->Height() * 1.5f / mGrid.getSize().y());
  mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y() * 1.2f / mGrid.getSize().y());
  mGrid.setColWidthPerc(0, (float)mSpace / mGrid.getSize().x());
  mGrid.setColWidthPerc(2, (float)mSpace / mGrid.getSize().x());

	// center for good measure
  setSize(mGrid.getSize());
	setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);

	addChild(&mGrid);

  // set accelerator automatically (button to press when "b" is pressed)
  if(mButtons.size() == 1)
  {
    mAcceleratorFunc = mButtons.front()->getPressedFunc();
  }else
  {
    for (auto& mButton : mButtons)
      if (String buttonUC = mButton->getText().ToUpperCase(); buttonUC == "OK" || buttonUC == "NO") // #TODO Do not rely on button texts
      {
        mAcceleratorFunc = mButton->getPressedFunc();
        break;
      }
  }

}

bool GuiMsgBoxScroll::ProcessInput(const InputCompactEvent& event)
{
	// special case for when GuiMsgBox comes up to report errors before anything has been configured
	/* when it's not configured, allow to remove the message box too to allow the configdevice window a chance */
	if(mAcceleratorFunc && event.AskForConfiguration())
	{
		mAcceleratorFunc();
		return true;
	}

	if (mMessage->ProcessInput(event)) return true;
	return Component::ProcessInput(event);
}

void GuiMsgBoxScroll::deleteMeAndCall(const std::function<void()>& func)
{
	Close();
	if(func) func();
}

GuiMsgBoxScroll* GuiMsgBoxScroll::SetDefaultButton(int index)
{
  if ((unsigned int)index < (unsigned int)mButtons.size())
    mButtonGrid->setCursorTo(mButtons[index]);

  return this;
}

bool GuiMsgBoxScroll::CollectHelpItems(Help& help)
{
	help.Clear();
	mGrid.CollectHelpItems(help);
	help.Set(HelpType::Joy2UpDown, _("Scroll manually"));
	return true;
}