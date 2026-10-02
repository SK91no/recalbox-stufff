#include "components/TextEditComponent.h"
#include "utils/Log.h"
#include "WindowManager.h"
#include "Renderer.h"

#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>

TextEditComponent::TextEditComponent(WindowManager&window, VKCharFilter filter)
	: Component(window)
  , mFocused(false)
  , mEditing(false)
  , mCursor(0)
  , mCursorRepeatTimer(0)
  , mCursorRepeatDir(0)
  , mTextWidth(0)
  , mTextHeight(0)
  , mScrollOffset(0.0f, 0.0f)
  , mBox(window, Path(":/textinput_ninepatch.png"))
  , mFont(ThemeManager::Instance().Menu().Text().font)
  , mFilter(filter)
{
	addChild(&mBox);
	
	onFocusLost();

	setSize(256, mFont->Height() + TEXT_PADDING_VERT);
}

void TextEditComponent::onFocusGained()
{
	mFocused = true;
	mBox.setImagePath(Path(":/textinput_ninepatch_active.png"));
	startEditing();
}

void TextEditComponent::onFocusLost()
{
	mFocused = false;
	mBox.setImagePath(Path(":/textinput_ninepatch.png"));
	stopEditing();
}

void TextEditComponent::onSizeChanged()
{
	mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-mBox.MargingX() * 2 - TEXT_PADDING_HORIZ, -mBox.MargingY() * 2));
	mScrollOffset[0] = 0;
	onTextChanged(); // wrap point probably changed
}

void TextEditComponent::setValue(const String& val)
{
  mText = val;
	onTextChanged();
}

void TextEditComponent::textInput(const char* text)
{
  //mText = text;
  //mCursor = cursor > 0 ? cursor : mText.length();
  mCursorRepeatDir = 0;
  if(text[0] == '\b')
  {
    if (mText.PreviousUTF8(mCursor))
      mText.DeleteUTF8(mCursor);
  }
  else
  {
    mText.Insert(mCursor, text);
    mCursor += strlen(text);
  }

	onTextChanged();
	onCursorChanged();
}

void TextEditComponent::startEditing()
{
	SDL_StartTextInput();
	mEditing = true;
  UpdateHelpBar();
}

void TextEditComponent::stopEditing()
{
	SDL_StopTextInput();
	mEditing = false;
  UpdateHelpBar();
}

void TextEditComponent::VirtualKeyboardValidated(IVirtualKeyboardBase& vk, const String& text)
{
  (void)vk;
  (void)text;
}

void TextEditComponent::VirtualKeyboardTextChange(IVirtualKeyboardBase& vk, const String& text)
{
  (void)vk;
  setValue(text);
  setCursor(text.length());
}

bool TextEditComponent::ProcessInput(const InputCompactEvent& event)
{
	if (event.AnythingReleased())
	{
		if (event.LeftReleased() || event.RightReleased())
			mCursorRepeatDir = 0;
		return false;
	}

	if (!event.IsKeyboard() && event.R1Pressed() && mFocused)
	{
    mWindow.CreateVirtualKeyboard("", getValue(), this, mFilter);
		//startEditing();
		return true;
	}

  if (event.IsKeyboard())
  {
    //{ LOG(LogDebug) << "[TextInput] Type: " << (int)event.RawEvent().Type() << " - Id: " << event.RawEvent().Id() << " - Device: " << event.RawEvent().Device() << " - Value: " << event.RawEvent().Value(); }
    bool pressed = event.RawEvent().Value() != 0;
    switch(event.RawEvent().Id())
    {
      case SDLK_BACKSPACE: if (pressed) textInput("\b"); break;
      case SDLK_DELETE:
      {
        if (pressed)
          if (mCursor < (int)mText.length())
          {
            mText.DeleteUTF8(mCursor);
            onTextChanged();
          }
        break;
      }
      case SDLK_RETURN:
      case SDLK_RETURN2: stopEditing(); break;
      case SDLK_ESCAPE: stopEditing(); break;
      case SDLK_LEFT:
      case SDLK_RIGHT:
      {
        mCursorRepeatDir = event.RawEvent().Id() == SDLK_LEFT ? -1 : 1;
        mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
        moveCursor(mCursorRepeatDir);
        break;
      }
    }
    return true;
  }

	return false;
}

void TextEditComponent::Update(int deltaTime)
{
	updateCursorRepeat(deltaTime);
  Component::Update(deltaTime);
}

void TextEditComponent::updateCursorRepeat(int deltaTime)
{
	if(mCursorRepeatDir == 0)
		return;

	mCursorRepeatTimer += deltaTime;
	while(mCursorRepeatTimer >= CURSOR_REPEAT_SPEED)
	{
		moveCursor(mCursorRepeatDir);
		mCursorRepeatTimer -= CURSOR_REPEAT_SPEED;
	}
}

void TextEditComponent::moveCursor(int amt)
{
  if (amt > 0) mText.DeleteUTF8(mCursor);
  else if (amt < 0) { if (mText.PreviousUTF8(mCursor)) mText.DeleteUTF8(mCursor); }
	onCursorChanged();
}

void TextEditComponent::setCursor(size_t pos)
{
	if (pos > mText.length())
		mCursor = (int)mText.length();
	else
		mCursor = (int)pos;

	moveCursor(0);
}

void TextEditComponent::onTextChanged()
{
	if(mCursor > (int)mText.length())
		mCursor = mText.length();

	if(mTextChangedCallback)
		mTextChangedCallback();
}

void TextEditComponent::onCursorChanged()
{
  Vector2f cursorPos = { (float)mFont->TextWidth(mText.SubString(0, mCursor)), 0 };

  if(mScrollOffset.x() + getTextAreaSize().x() < cursorPos.x())
  {
    mScrollOffset[0] = cursorPos.x() - getTextAreaSize().x();
  }else if(mScrollOffset.x() > cursorPos.x())
  {
    mScrollOffset[0] = cursorPos.x();
  }
}

void TextEditComponent::Render(const Transform4x4f& parentTrans)
{
  if (mThemeDisabled) return;

	Transform4x4f trans = getTransform() * parentTrans;
	renderChildren(trans);

	// text + cursor rendering
	// offset into our "text area" (padding)
	trans.translation() += Vector3f(getTextAreaPos().x(), getTextAreaPos().y(), 0);

  Renderer::Instance().Clip(trans, mSize);

  //Vector2f position = getTextAreaPos();
  //mFont->RenderDirect(mText, position.x() - (int)mScrollOffset.x(), position.y() - (int)mScrollOffset.y(), 0x77777700 | getOpacity());

	Renderer::SetMatrix(trans);

  mFont->DrawText(mText, Rectangle(0.f, 0.f, getTextAreaSize().x(), getTextAreaSize().y()), 0x77777700 | getOpacity(), ::Alignment::CenterLeft);

	// pop the clip early to allow the cursor to be drawn outside of the "text area"
	Renderer::Instance().Unclip();

	// draw cursor
	if(mEditing)
	{
		Vector2f cursorPos(0);
    cursorPos.x() = mFont->TextWidth(mText.SubString(0, mCursor));
		cursorPos.y() = 0;

		float cursorHeight = mFont->Height() * 0.8f;
		Renderer::DrawRectangle(cursorPos.x(), cursorPos.y() + (mFont->Height() - cursorHeight) / 2, 2.0f, cursorHeight, 0x000000FF);
	}
}

Vector2f TextEditComponent::getTextAreaSize() const
{
	return { mSize.x() - TEXT_PADDING_HORIZ, mSize.y() - TEXT_PADDING_VERT };
}

bool TextEditComponent::CollectHelpItems(Help& help)
{
	if(mEditing)
		help.Set(HelpType::AllDirections, _("MOVE CURSOR"))
		    .Set(Help::Cancel(), _("STOP EDITING"));
	else
		help.Set(Help::Valid(), _("EDIT"));
	return true;
}

/*Vector2f TextEditComponent::getTextAreaSize() const
{
  return { mSize.x() - (TEXT_PADDING_HORIZ + mBox.MargingX() * 2), mSize.y() - (TEXT_PADDING_VERT + mBox.MargingY() * 2) };
}

Vector2f TextEditComponent::getTextAreaPos()
{
  Vector2f result( mBox.MargingX() + TEXT_PADDING_HORIZ / 2.0f, mBox.MargingY() + TEXT_PADDING_VERT / 2.0f );
  return result;
}*/

int TextEditComponent::getTextHeight() const
{
  int h = Math::roundi(mFont->Height() * 1.5f);
  return h;
}

int TextEditComponent::getVerticalMargins() const
{
  return mBox.MargingY() * 2 + TEXT_PADDING_VERT;
}
