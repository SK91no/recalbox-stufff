#include <components/ComponentList.h>
#include <themes/MenuThemeData.h>
#include <help/Help.h>
#include <utils/locale/LocaleHelper.h>
#include "themes/ThemeManager.h"

#define TOTAL_HORIZONTAL_PADDING_PX 20

ComponentList::ComponentList(WindowManager&window)
   : IList<ComponentListRow, void*>(window, LIST_SCROLL_STYLE_SLOW, LoopType::NeverLoop)
{
	mSelectorBarOffset = 0;
	mCameraOffset = 0;
	mFocused = false;
}

void ComponentList::addRow(const ComponentListRow& row, bool setCursorHere, bool updateGeometry)
{
	IList<ComponentListRow, void*>::Entry e;
	//e.name = row.name;
	e.object = nullptr;
	e.data = row;

	this->add(e);

	for (auto& element : mEntries.back().data.Elements())
		addChild(element.mComponent.get());

	if (updateGeometry) {
		updateElementSize(mEntries.back().data);
		updateElementPosition(mEntries.back().data);
	}

	if(setCursorHere)
	{
		mCursor = (int)mEntries.size() - 1;
		onCursorChanged(CursorState::Stopped);
	}
}

void ComponentList::onSizeChanged()
{
	for (auto& mEntrie : mEntries)
	{
		updateElementSize(mEntrie.data);
		updateElementPosition(mEntrie.data);
	}

	updateCameraOffset();
}

void ComponentList::onFocusLost()
{
	mFocused = false;

	if(mFocusLostCallback)
		mFocusLostCallback();
}

void ComponentList::onFocusGained()
{
	mFocused = true;

	if(mFocusGainedCallback)
		mFocusGainedCallback();
}

bool ComponentList::ProcessInput(const InputCompactEvent& event)
{
	if(size() == 0)
		return false;

	if (mEntries[mCursor].data.ProcessInput(event))
	  return true;

	// input handler didn't consume the input - try to scroll
	if (event.AnyUpPressed())   return listInput(-1);
	if (event.AnyDownPressed()) return listInput(1);
	if (event.L1Pressed())      return listInput(-7);
	if (event.R1Pressed())      return listInput(7);
	if (event.AnyUpReleased() || event.AnyDownReleased() || event.L1Released() || event.R1Released())
	  listInput(0);

	return false;
}

void ComponentList::Update(int deltaTime)
{
	listUpdate(deltaTime);

	if(size() != 0)
	{
		// update our currently selected row
		for (auto& element : mEntries[mCursor].data.Elements())
      element.mComponent->Update(deltaTime);
	}
}

void ComponentList::onCursorChanged(const CursorState& state)
{
	// update the selector bar position
	// in the future this might be animated
	mSelectorBarOffset = 0;
	for (int i = 0; i < mCursor; i++)
	{
		mSelectorBarOffset += getRowHeight(mEntries[i].data);
	}

	updateCameraOffset();

	// this is terribly inefficient but we don't know what we came from so...
	if(size() != 0)
	{
		for (auto& mEntrie : mEntries)
			mEntrie.data.Elements().back().mComponent->onFocusLost();
		
		mEntries[mCursor].data.Elements().back().mComponent->onFocusGained();
	}

	if(mCursorChangedCallback)
		mCursorChangedCallback(state);

  UpdateHelpBar();
}

void ComponentList::updateCameraOffset()
{
	// move the camera to scroll
	const float totalHeight = getTotalRowHeight();
	if(totalHeight > mSize.y() && !mEntries.empty())
	{
		float target = mSelectorBarOffset + getRowHeight(mEntries[mCursor].data)/2 - (mSize.y() / 2);

		// clamp it
		mCameraOffset = 0;
		unsigned int i = 0;
		while(mCameraOffset < target && i < mEntries.size())
		{
			mCameraOffset += getRowHeight(mEntries[i].data);
			i++;
		}

		if(mCameraOffset < 0)
			mCameraOffset = 0;
		else if(mCameraOffset + mSize.y() > totalHeight)
			mCameraOffset = totalHeight - mSize.y();
	}else{
		mCameraOffset = 0;
	}
}

void ComponentList::Render(const Transform4x4f& parentTrans)
{
	if(size() == 0) return;

  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
	unsigned int selectorColor = menuTheme.Text().selectorColor;
	unsigned int selectedColor = menuTheme.Text().selectedColor;
	unsigned int bgColor = menuTheme.Background().color;
	unsigned int separatorColor = menuTheme.Text().separatorColor;

	Transform4x4f trans = (parentTrans * getTransform()).round();

	// clip everything to be inside our bounds
  Renderer::Instance().Clip(trans, mSize);

	// scroll the camera
	trans.translate(Vector3f(0, -Math::round(mCameraOffset), 0));

	// draw our entries
	std::vector<Component*> drawAfterCursor;
	for (int i = 0; i < (int)mEntries.size(); i++)
	{
		auto& entry = mEntries[i];
		bool drawAll = !mFocused || i != mCursor;
		for (auto& element : entry.data.Elements())
		{
			if(drawAll || element.mInvertWhenSelected)
			{
				element.mComponent->setColor(element.mComponent->getOriginColor());
        element.mComponent->Render(trans);
			}else{
				drawAfterCursor.push_back(element.mComponent.get());
			}
		}
	}

	// custom rendering
	Renderer::SetMatrix(trans);

	// draw selector bar	
	if(mFocused)
	{			
		const float selectedRowHeight = getRowHeight(mEntries[mCursor].data);
		//here we draw a bar then redraw the list entry
		if ((selectorColor != bgColor) && ((selectorColor & 0xFF) != 0x00)) {
			Renderer::DrawRectangle(0.0f, mSelectorBarOffset, mSize.x(), selectedRowHeight, bgColor, BlendMode::InverseMultiply);
			Renderer::DrawRectangle(0.0f, mSelectorBarOffset, mSize.x(), selectedRowHeight, selectorColor, BlendMode::Additive);
		}
		auto& entry = mEntries[mCursor];
		for (auto& element : entry.data.Elements())
		{
			element.mComponent->setColor(selectedColor);
			drawAfterCursor.push_back(element.mComponent.get());
		}

		for (auto& it : drawAfterCursor)
      it->Render(trans);
		
		// reset matrix if one of these components changed it
		if (!drawAfterCursor.empty())
			Renderer::SetMatrix(trans);
	}

	// draw separators
	float y = 0;
	
	for (auto& mEntrie : mEntries)
	{
		Renderer::DrawRectangle(0.0f, y, mSize.x(), 1.0f, separatorColor);
		y += getRowHeight(mEntrie.data);
	}
	Renderer::DrawRectangle(0.0f, y, mSize.x(), 1.0f, separatorColor);

  Renderer::Instance().Unclip();
}

float ComponentList::getRowHeight(const ComponentListRow& row)
{
	// returns the highest component height found in the row
	float height = 0;
	for (const auto& element : row.Elements())
	{
		if(element.mComponent->getSize().y() > height)
			height = element.mComponent->getSize().y();
	}

	return height + 2;
}

float ComponentList::getTotalRowHeight() const
{
	float height = 0;
	for (const auto& mEntrie : mEntries)
	{
		height += getRowHeight(mEntrie.data);
	}

	return height;
}

void ComponentList::updateElementPosition(const ComponentListRow& row)
{
	float yOffset = 0;
	for (auto it = mEntries.begin(); it != mEntries.end() && &it->data != &row; it++)
	{
		yOffset += getRowHeight(it->data);
	}

	// assumes updateElementSize has already been called
	float rowHeight = getRowHeight(row);

	float x = (float)TOTAL_HORIZONTAL_PADDING_PX / 2;
	for (const auto& element : row.Elements())
	{
		const auto comp = element.mComponent;

		// center vertically
		comp->setPosition(x, (rowHeight - comp->getSize().y()) / 2 + yOffset);
		x += comp->getSize().x();
	}
}

void ComponentList::updateElementSize(const ComponentListRow& row)
{
	float width = mSize.x() - TOTAL_HORIZONTAL_PADDING_PX;
	std::vector< std::shared_ptr<Component> > resizeVec;

	for (const auto& element : row.Elements())
	{
		if(element.mResizeWidth)
			resizeVec.push_back(element.mComponent);
		else
			width -= element.mComponent->getSize().x();
	}

	// redistribute the "unused" width equally among the components with resize_width set to true
	width = width / resizeVec.size();
	for (auto& it : resizeVec)
	{
		it->setSize(width, it->getSize().y());
	}
}

void ComponentList::textInput(const char* text)
{
	if(size() == 0)
		return;

	mEntries[mCursor].data.Elements().back().mComponent->textInput(text);
}

bool ComponentList::CollectHelpItems(Help& help)
{
	if (size() == 0) return false;
  mEntries[mCursor].data.Elements().back().mComponent->CollectHelpItems(help);

	if (size() > 1)
		if (help.IsSet(HelpType::UpDown) || help.IsSet(HelpType::AllDirections)) help.Set(HelpType::UpDown, _("CHOOSE"));

	if ((mEntries[mCursor].data.HasHelpHandler()) && RecalboxConf::Instance().GetPopupHelp() != 0)
		help.Set(HelpType::Y, _("HELP"));

	return true;
}

bool ComponentList::moveCursor(int amt)
{
	bool ret = listInput(amt); 
	listInput(0); 
	return ret;
}
