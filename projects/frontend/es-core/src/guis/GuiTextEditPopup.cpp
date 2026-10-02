#include <guis/GuiTextEditPopup.h>
#include <components/MenuComponent.h>
#include <components/ButtonComponent.h>
#include <utils/locale/LocaleHelper.h>
#include <themes/MenuThemeData.h>

GuiTextEditPopup::GuiTextEditPopup(WindowManager& window, const String& title, const String& initValue,
                                   const std::function<void(const String&)>& okCallback, bool multiLine,
                                   const String& acceptBtnText)
	: Gui(window),
	  mGrid(window, Vector2i(1, 3)),
	  mMultiLine(multiLine)
{
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	
	addChild(&mGrid);

	mTitle = std::make_shared<TextComponent>(mWindow, title.ToUpperCaseUTF8(), menuTheme.Title().font, menuTheme.Title().color, ::Alignment::Center);

	mText = std::make_shared<TextEditComponent>(mWindow);
	mText->setValue(initValue);

	if(!multiLine)
		mText->setCursor(initValue.size());

	std::vector< std::shared_ptr<ButtonComponent> > buttons;
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, acceptBtnText, acceptBtnText, [this, okCallback] { okCallback(mText->getValue()); Close(); }));
	buttons.push_back(std::make_shared<ButtonComponent>(mWindow, _("CANCEL"), _("DISCARD CHANGES"), [this] { Close(); }));

	mButtonGrid = MenuComponent::MakeButtonGrid(mWindow, buttons);

	mGrid.setEntry(mTitle, Vector2i(0, 0), false, true);
	mGrid.setEntry(mText, Vector2i(0, 1), true, false, Vector2i(1, 1), Borders::Top | Borders::Bottom);
	mGrid.setEntry(mButtonGrid, Vector2i(0, 2), true, false);

	float textHeight = mText->getFont()->Height();
	if(multiLine)
		textHeight *= 6;
	mText->setSize(0, textHeight);

	setSize(Renderer::Instance().DisplayWidthAsFloat() * 0.5f, mTitle->getFont()->Height() + textHeight + mButtonGrid->getSize().y() + 40);
	setPosition((Renderer::Instance().DisplayWidthAsFloat() - mSize.x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mSize.y()) / 2);
}

void GuiTextEditPopup::onSizeChanged()
{
	mText->setSize(mSize.x() - 40, mText->getSize().y());

	// update grid
	mGrid.setRowHeightPerc(0, mTitle->getFont()->Height() / mSize.y());
	mGrid.setRowHeightPerc(2, mButtonGrid->getSize().y() / mSize.y());
	mGrid.setSize(mSize);
}

bool GuiTextEditPopup::ProcessInput(const InputCompactEvent& event)
{
	if (Component::ProcessInput(event))
		return true;

	// pressing back when not text editing closes us
	if (event.CancelReleased())
	{
		Close();
		return true;
	}

	return false;
}

bool GuiTextEditPopup::CollectHelpItems(Help& help)
{
  mGrid.CollectHelpItems(help);
	help.Set(Help::Cancel(), _("BACK"));
	return true;
}
