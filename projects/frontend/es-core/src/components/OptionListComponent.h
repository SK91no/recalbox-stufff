#pragma once

#include "components/base/Component.h"
#include "WindowManager.h"
#include "components/TextComponent.h"
#include "components/ImageComponent.h"
#include "components/MenuComponent.h"
#include "utils/Log.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include "IOptionListComponent.h"
#include "IOptionListMultiComponent.h"

//Used to display a list of options.
//Can select one or multiple options.

// if !multiSelect
// * <- curEntry ->

// always
// * press a -> open full list

#define CHECKED_PATH ":/checkbox_checked.svg"
#define UNCHECKED_PATH ":/checkbox_unchecked.svg"

template<typename T>
class OptionListComponent : public Component
{
private:
	struct OptionListData
	{
		String name;
		String value;
		T object;
		bool selected = false;
	};

	class OptionListPopup : public Gui
	{
	private:
		MenuComponent mMenu;
		OptionListComponent<T>* mParent;

	public:
		OptionListPopup(WindowManager& window, OptionListComponent<T>* parent, const String& title)
		  : Gui(window),
			  mMenu(window, title),
			  mParent(parent)
		{
      addChild(&mMenu);

      const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
			auto font = menuTheme.Text().font;
			auto color = menuTheme.Text().color;
			ComponentListRow row;

			// for select all/none
			std::vector<ImageComponent*> checkboxes;

      for (auto& e : mParent->mEntries)
			{
				row.Clear();
				row.addElement(std::make_shared<TextComponent>(mWindow, e.name.ToUpperCaseUTF8(), font, color), true);

				if(mParent->mMultiSelect)
				{
					// add checkbox
					auto checkbox = std::make_shared<ImageComponent>(mWindow);
					checkbox->setImage(Path(e.selected ? CHECKED_PATH : UNCHECKED_PATH), false);
					checkbox->setResize(0, font->Height());
					checkbox->setColor(color);
					row.addElement(checkbox, false);

					// input handler
					// update checkbox state & selected value
					row.makeAcceptInputHandler([this, &e, checkbox, color]
					{
						e.selected = !e.selected;
						checkbox->setImage(Path(e.selected ? CHECKED_PATH : UNCHECKED_PATH), false);
						checkbox->setColor(color);
						mParent->onSelectedChanged(false);
					});

					// for select all/none
					checkboxes.push_back(checkbox.get());
				}else{
					// input handler for non-multiselect
					// update selected value and close
					row.makeAcceptInputHandler([this, &e]
					{
						mParent->mEntries[mParent->getSelectedId()].selected = false;
						e.selected = true;
						mParent->onSelectedChanged(false);
						Close();
					});
				}

				// also set cursor to this row if we're not multi-select and this row is selected
				mMenu.addRow(row, (!mParent->mMultiSelect && e.selected), false);
			}

			if(mParent->mMultiSelect)
			{
			  mMenu.addButton(_("SELECT ALL"), "select all", [this, checkboxes, color]
			  {
					for (unsigned int i = 0; i < mParent->mEntries.size(); i++)
					{
						mParent->mEntries[i].selected = true;
						checkboxes[i]->setImage(Path(CHECKED_PATH), false);
						checkboxes[i]->setColor(color);
					}
					mParent->onSelectedChanged(false);
				});

			  mMenu.addButton(_("SELECT NONE"), "select none", [this, checkboxes, color]
			  {
					for (unsigned int i = 0; i < mParent->mEntries.size(); i++)
					{
						mParent->mEntries[i].selected = false;
						checkboxes[i]->setImage(Path(UNCHECKED_PATH), false);
						checkboxes[i]->setColor(color);
					}
					mParent->onSelectedChanged(false);
				});
			}

      mMenu.refresh();
			mMenu.setPosition((Renderer::Instance().DisplayWidthAsFloat() - mMenu.getSize().x()) / 2, (Renderer::Instance().DisplayHeightAsFloat() - mMenu.getSize().y()) / 2);
		}

		bool ProcessInput(const InputCompactEvent& event) override
		{
			if (event.CancelReleased())
			{
				Close();
				return true;
			}

			return Component::ProcessInput(event);
		}

		bool CollectHelpItems(Help& help) override
		{
      mMenu.CollectHelpItems(help);
			help.Set(Help::Cancel(), _("BACK"));
			return true;
		}
	};

public:
	OptionListComponent(WindowManager& window, const String& name, bool multiSelect, unsigned int font_size)
	  : Component(window)
    , mName(name)
    , mText(window)
    , mLeftArrow(window)
    , mRightArrow(window)
    , mOriginColor(0)
    , mId(0)
    , mInterface(nullptr)
    , mMultiInterface(nullptr)
    , mMultiSelect(multiSelect)
	{
    const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
    NewFont* font = (font_size == FONT_SIZE_SMALL) ? menuTheme.SmallText().font : menuTheme.Text().font;
    unsigned int color = menuTheme.Text().color;
		mOriginColor = color;
			
		mText.setFont(font);
		mText.setColor(color);
		mText.setHorizontalAlignment(::HorizontalAlignment::Center);
		addChild(&mText);

		if(mMultiSelect)
		{
			mRightArrow.setImage(menuTheme.Elements().FromType(MenuThemeData::IconElement::Type::Arrow), false);
			mRightArrow.setColorShift(color);
			addChild(&mRightArrow);
		}else{
			mLeftArrow.setImage(menuTheme.Elements().FromType(MenuThemeData::IconElement::Type::OptionArrow), false);
			mLeftArrow.setColorShift(color);
			mLeftArrow.setFlipX(true);
			addChild(&mLeftArrow);

			mRightArrow.setImage(menuTheme.Elements().FromType(MenuThemeData::IconElement::Type::OptionArrow), false);
			mRightArrow.setColorShift(color);
			addChild(&mRightArrow);
		}

		setSize(mLeftArrow.getSize().x() + mRightArrow.getSize().x(), font->Height());
	}

  OptionListComponent(WindowManager&window, const String& name, bool multiSelect)
    : OptionListComponent(window, name, multiSelect, FONT_SIZE_MEDIUM)
  {
  }

  OptionListComponent(WindowManager&window, const String& name)
    : OptionListComponent(window, name, false, FONT_SIZE_MEDIUM)
  {
  }

  // handles positioning/resizing of text and arrows
	void onSizeChanged() override
	{
		mLeftArrow.setResize(0, mText.getFont()->Height());
		mRightArrow.setResize(0, mText.getFont()->Height());

    mText.setSize(mSize.x() - mLeftArrow.getSize().x() - mRightArrow.getSize().x(), mText.getFont()->Height());

		// position
		mLeftArrow.setPosition(0, (mSize.y() - mLeftArrow.getSize().y()) / 2);
		mText.setPosition(mLeftArrow.getPosition().x() + mLeftArrow.getSize().x(), (mSize.y() - mText.getSize().y()) / 2);
		mRightArrow.setPosition(mText.getPosition().x() + mText.getSize().x(), (mSize.y() - mRightArrow.getSize().y()) / 2);
	}

	bool ProcessInput(const InputCompactEvent& event) override
	{
    if (event.ValidReleased())
    {
      open();
      return true;
    }
    if(!mMultiSelect)
    {
      if (event.AnyLeftReleased())
      {
        // move selection to previous
        unsigned int i = getSelectedId();
        int next = (int)i - 1;
        if(next < 0)
          next += mEntries.size();

        mEntries[i].selected = false;
        mEntries[next].selected = true;
        onSelectedChanged(true);
        return true;

      }else if (event.AnyRightReleased())
      {
        // move selection to next
        unsigned int i = getSelectedId();
        int next = (i + 1) % mEntries.size();
        mEntries[i].selected = false;
        mEntries[next].selected = true;
        onSelectedChanged(true);
        return true;

      }
    }
		return Component::ProcessInput(event);
	}

	std::vector<T> getSelectedObjects()
	{
		std::vector<T> ret;
		for (auto& comp : mEntries)
		{
			if(comp.selected)
				ret.push_back(comp.object);
		}

		return ret;
	}

  Array<T> getSelectedObjectsAsArray()
  {
    Array<T> ret;
    for (auto& comp : mEntries)
    {
      if(comp.selected)
        ret.Add(comp.object);
    }

    return ret;
  }

  T getSelected()
	{
		assert(!mMultiSelect);
		auto selected = getSelectedObjects();
		if(selected.size() == 1) return selected[0];
    return T();
	}
        
	String getSelectedName()
	{
    assert(!mMultiSelect);
		for (auto& entry : mEntries)
		{
			if(entry.selected)
				return entry.name;
		}
    return "";
	}

	void select(T object) {
	    bool hasChanged = false;
	    bool selectable = true;
		for (auto& entry : mEntries) {
		    const bool previous = entry.selected;
			entry.selected = selectable && (entry.object == object);
			if (entry.selected && !mMultiSelect) {
                selectable = false;
			}
            hasChanged = hasChanged || (entry.selected != previous);
		}
		if (hasChanged) {
            onSelectedChanged(false);
		}
	}
        
	void clear() { 
		mEntries.clear();
	}

    void add(const String& name, const T& obj, bool selected, const String& value = "")
        {
		OptionListData e;
		e.name = name;
		e.value = value;
		e.object = obj;
		e.selected = selected;
		if (selected) {
			if (!mMultiSelect) {
				for (auto& entry : mEntries) {
					entry.selected = false;
				}
			}
			firstSelected = obj;
		}

		mEntries.push_back(e);
		onSelectedChanged(false);
	}

	inline void invalidate() {
		onSelectedChanged(false);
	}

	inline void setSelectedChangedCallback(const std::function<void(const T&)>& callback) {
		mSelectedChangedCallback = callback;
	}

	inline void setChangedCallback(const std::function<void()>& callback) { mChangedCallback = callback; };

  inline void SetInterface(int id, IOptionListComponent<T>* interface)
  {
    mId = id;
    mInterface = interface;
  }

  inline void SetMultiInterface(int id, IOptionListMultiComponent<T>* interface)
  {
    mId = id;
    mMultiInterface = interface;
  }

  bool changed() {return firstSelected != getSelected();}

  unsigned int getSelectedIndex() const { return getSelectedId(); }

  void setSelectedIndex(int index)
  {
    if ((unsigned int)index < mEntries.size() && !mMultiSelect)
    {
      mEntries[getSelectedIndex()].selected = false;
      mEntries[index].selected = true;
      onSelectedChanged(false);
    }
  }

private:
	unsigned int getSelectedId() const
	{
		assert(!mMultiSelect);
		for (unsigned int i = 0; i < mEntries.size(); i++)
		{
			if(mEntries[i].selected)
				return i;
		}

    { LOG(LogWarning) << "[OptionListComponent] no selected element found, defaulting to 0"; }
		return 0;
	}

  void open()
	{
		mWindow.pushGui(new OptionListPopup(mWindow, this, mName));
	}

	void onSelectedChanged(bool quickChange)
	{
		if(mMultiSelect)
		{
			// display # selected
			int x = getSelectedObjects().size();
			mText.setText(_N("%i SELECTED", "%i SELECTED", x).Replace("%i", String(x)));
			mText.setSize(0, mText.getSize().y());
			setSize(mText.getSize().x() + mRightArrow.getSize().x() + 24, mText.getSize().y());
			if(mParent) // hack since theres no "on child size changed" callback atm...
				mParent->onSizeChanged();
		}else{
			// display currently selected + l/r cursors
			for (const auto& entry : mEntries)
			{
				if(entry.selected)
				{
					mText.setText(entry.name.ToUpperCaseUTF8());
					mText.setSize(0, mText.getSize().y());
					setSize(mText.getSize().x() + mLeftArrow.getSize().x() + mRightArrow.getSize().x() + 24, mText.getSize().y());
					if(mParent) // hack since theres no "on child size changed" callback atm...
						mParent->onSizeChanged();
          onSizeChanged();
					break;
				}
			}
		}

		if (mInterface != nullptr)
      mInterface->OptionListComponentChanged(mId, getSelectedId(), mEntries[getSelectedId()].object, quickChange);

    if (mMultiInterface != nullptr)
    {
      mMultiInterface->OptionListMultiComponentChanged(mId, getSelectedObjects());
    }

    if (mSelectedChangedCallback) {
			mSelectedChangedCallback(mEntries[getSelectedId()].object);
		}

		if (mChangedCallback) {
			mChangedCallback();
		}
	}

	bool CollectHelpItems(Help& help) override
	{
		if(!mMultiSelect)
		  help.Set(HelpType::LeftRight, _("CHANGE"));
		
		help.Set(Help::Valid(), _("SELECT"));
		return true;
	}

	void setColor(unsigned int color) override
	{
		mText.setColor(color);
		mRightArrow.setColorShift(color);
		mLeftArrow.setColorShift(color);
	}

	inline void setOriginColor(unsigned int color){mOriginColor = color;};
	inline unsigned int getOriginColor() override{return mOriginColor;};

    [[nodiscard]] String getValue() const override {
        assert(!mMultiSelect);
        for (auto& entry : mEntries)
        {
            if(entry.selected) {
                return entry.value;
            }
        }
        return String::Empty;
    }

	String mName;
	T firstSelected;
	TextComponent mText;
	ImageComponent mLeftArrow;
	ImageComponent mRightArrow;
	unsigned int mOriginColor;
	int mId;

	std::vector<OptionListData> mEntries;
	std::function<void(const T&)> mSelectedChangedCallback;
	std::function<void()> mChangedCallback;
	IOptionListComponent<T>* mInterface;
  IOptionListMultiComponent<T>* mMultiInterface;

  bool mMultiSelect;
};
