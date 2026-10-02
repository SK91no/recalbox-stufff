#include "components/ScraperSearchComponent.h"

#include "components/TextComponent.h"
#include "components/MarkdownText.h"
#include "components/ImageComponent.h"
#include "components/RatingComponent.h"
#include "components/DateTimeComponent.h"
#include "components/AnimatedImageComponent.h"
#include "utils/Log.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"
#include "utils/locale/LocaleHelper.h"
#include "themes/MenuThemeData.h"
#include <themes/ThemeManager.h>

#include "hardware/Board.h"

ScraperSearchComponent::ScraperSearchComponent(WindowManager& window, bool lowResolution)
  : Component(window)
  , mGrid(window, Vector2i(7, 8))
  , mLowResolution(lowResolution)
  , mBusyAnim(window)
{
	addChild(&mGrid);
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();

	// GRID: 7 x 6
	// +-+-------------------------------------------------------------------------------------------------------------+-+
	// | | GAME TITLE                                                                                                  | | 0 = 10% or Text height
  // | +-------------------------------+--------------------+-------------------+-------------------+----------------+ |
  // | |  ###########################  |        DEVELOPER : | SEGA              |          RATING : | @@@OO          | | 1 = 10% or Text height
  // | |  #                         #  +--------------------+-------------------+-------------------+----------------+ |
  // | |  #                         #  |        PUBLISHER : | NINTENDO          |    RELEASE DATE : | 1891/12/03     | | 2 = 10% or Text height
  // | |  #                         #  +--------------------+-------------------+-------------------+----------------+ |
  // | |  #                         #  |            GENRE : | SHOOT'EM-UP       |         PLAYERS : | 1-2            | | 3 = 10% or Text height
  // | |  #                         #  +--------------------+-------------------+-------------------+----------------+ |
  // | |  #                         #  |                                                                             | |
  // | |  #                         #  | Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod     | |
  // | |  #                         #  | tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim        | | 4 = 30%
  // | |  ###########################  | veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea     | |
  // | |                               | commodo consequat. Duis aute irure dolor in reprehenderit in voluptate      | |
  // | +-------------------------------+ velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat  | |
  // | |                               | cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id   | |
  // | |                               | est laborum.                                                                | |
  // | |   @O Animated busy            |                                                                             | |
  // | |   OO                          |                                                                             | | 5 = 30%
  // | |                               |                                                                             | |
  // | |                               |                                                                             | |
  // +-+-------------------------------+-----------------------------------------------------------------------------+-+
  //  0              1                            2                  3                   4                   5        6
  //  5%            30%                          15%                15%                 15%                 15%       5%

  NewFont* font = menuTheme.SmallText().font;
  const unsigned int mdColor = menuTheme.Text().color;
  const unsigned int mdLblColor = menuTheme.Text().color;

  // Game Name
  mResultName = std::make_shared<TextComponent>(mWindow, "RESULT NAME", menuTheme.Text().font, menuTheme.Text().color);
  mGrid.setEntry(mResultName, Vector2i(1, 0), false, true, Vector2i(5, 1));

  // Image thumbnail
  mResultThumbnail = std::make_shared<PictureComponent>(mWindow, ImgProps::KeepRatio | ImgProps::NoCache);
  mGrid.setEntry(mResultThumbnail, lowResolution ? Vector2i(4,1) : Vector2i(1, 1), false, true, Vector2i(lowResolution ? 2 : 1, lowResolution ? 7 : 6));

  // selected result desc + container
  mResultDesc = std::make_shared<MarkdownText>(mWindow, "", menuTheme.Text().color, Alignment::TopLeft);
  // show description on the right
  if(!lowResolution)
    mGrid.setEntry(mResultDesc, Vector2i(2, 4), false, true, Vector2i(4, 4));

  // Labels
  mLabelDeveloper = std::make_shared<TextComponent>(mWindow, _("Developer").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mLabelPublisher = std::make_shared<TextComponent>(mWindow, _("Publisher").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mLabelGenre = std::make_shared<TextComponent>(mWindow, _("Genre").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mLabelRating = std::make_shared<TextComponent>(mWindow, _("Rating").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mLabelReleaseDate = std::make_shared<TextComponent>(mWindow, _("Released").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mLabelPlayers = std::make_shared<TextComponent>(mWindow, _("Players").UpperCaseUTF8().Append(':'), font, mdLblColor, ::Alignment::CenterRight);
  mGrid.setEntry(mLabelDeveloper, Vector2i(2, 1), false, true, Vector2i(1, 1));
  mGrid.setEntry(mLabelPublisher, Vector2i(2, 2), false, true, Vector2i(1, 1));
  mGrid.setEntry(mLabelGenre, Vector2i(2, 3), false, true, Vector2i(1, 1));
  mGrid.setEntry(mLabelRating, lowResolution ? Vector2i(2,4) : Vector2i(4, 1), false, true, Vector2i(1, 1));
  mGrid.setEntry(mLabelReleaseDate, lowResolution ? Vector2i(2,5) : Vector2i(4, 2), false, true, Vector2i(1, 1));
  mGrid.setEntry(mLabelPlayers, lowResolution ? Vector2i(2,6) : Vector2i(4, 3), false, true, Vector2i(1, 1));

  // Value
  mValueDeveloper = std::make_shared<TextScrollComponent>(mWindow, "", font, mdColor, ::Alignment::CenterLeft);
  mValuePublisher = std::make_shared<TextScrollComponent>(mWindow, "", font, mdColor, ::Alignment::CenterLeft);
  mValueGenre = std::make_shared<TextScrollComponent>(mWindow, "", font, mdColor, ::Alignment::CenterLeft);
  mValueRating = std::make_shared<RatingComponent>(mWindow, menuTheme.Text().color, 0.f, "scraper");
  mValueReleaseDate = std::make_shared<DateTimeComponent>(mWindow);
  mValueReleaseDate->setColor(mdColor);
  mValueReleaseDate->setHorizontalAlignment(::HorizontalAlignment::Left);
  mValuePlayers = std::make_shared<TextScrollComponent>(mWindow, "", font, mdColor, ::Alignment::CenterLeft);
  mGrid.setEntry(mValueDeveloper, Vector2i(3, 1), false, true, Vector2i(1, 1));
  mGrid.setEntry(mValuePublisher, Vector2i(3, 2), false, true, Vector2i(1, 1));
  mGrid.setEntry(mValueGenre, Vector2i(3, 3), false, true, Vector2i(1, 1));
  mGrid.setEntry(mValueRating, lowResolution ? Vector2i(3, 4) : Vector2i(5, 1), false, true, Vector2i(1, 1));
  mGrid.setEntry(mValueReleaseDate, lowResolution ? Vector2i(3, 5) : Vector2i(5, 2), false, true, Vector2i(1, 1));
  mGrid.setEntry(mValuePlayers, lowResolution ? Vector2i(3, 6) : Vector2i(5, 3), false, true, Vector2i(1, 1));

  mRunning = true;
}

void ScraperSearchComponent::onSizeChanged()
{
	mGrid.setSize(mSize);
	
	if(mSize.x() == 0 || mSize.y() == 0) return;

  // column widths
  if(mLowResolution)
  {
    mGrid.setColWidthPerc(0, 0.05f);
    mGrid.setColWidthPerc(1, 0.00f);
    mGrid.setColWidthPerc(2, 0.25f);
    mGrid.setColWidthPerc(3, 0.25f);
    mGrid.setColWidthPerc(4, 0.20f);
    mGrid.setColWidthPerc(5, 0.20f);
    mGrid.setColWidthPerc(6, 0.05f);
  } else {
    mGrid.setColWidthPerc(0, 0.05f);
    mGrid.setColWidthPerc(1, 0.30f);
    mGrid.setColWidthPerc(2, 0.15f);
    mGrid.setColWidthPerc(3, 0.15f);
    mGrid.setColWidthPerc(4, 0.15f);
    mGrid.setColWidthPerc(5, 0.15f);
    mGrid.setColWidthPerc(6, 0.05f);
  }

	// row heights
  const MenuThemeData& menuTheme = ThemeManager::Instance().Menu();
  NewFont* font = menuTheme.SmallText().font;
	float firstRowPercent = ((float)mResultName->getFont()->Height() * 1.6f) / mGrid.getSize().y();
	float coeff = 1.6f;
	if (Renderer::Instance().Is240p() && Board::Instance().CrtBoard().IsCrtAdapterAttached()) coeff = 1.4f;
	else if (Renderer::Instance().Is480pOrLower()) coeff = 1.0f;
  float textRowPercent  = ((float)font->Height() * coeff) / mGrid.getSize().y();
	mGrid.setRowHeightPerc(0, firstRowPercent); // result name
  mGrid.setRowHeightPerc(1, textRowPercent);
  mGrid.setRowHeightPerc(2, textRowPercent);
  mGrid.setRowHeightPerc(3, textRowPercent);
  mGrid.setRowHeightPerc(4, textRowPercent);
  mGrid.setRowHeightPerc(5, textRowPercent);
  mGrid.setRowHeightPerc(6, textRowPercent);
  mGrid.setRowHeightPerc(7, 0);

  // Resize title & description
	//const float boxartCellScale = 0.9f;
	//mResultThumbnail->setSize(mGrid.getColWidth(1) * boxartCellScale, mGrid.getRowHeight(1, 4) * boxartCellScale);
  mResultThumbnail->setKeepRatio(true);
	//mDescContainer->setSize(mGrid.getColWidth(2, 5) * boxartCellScale, mGrid.getRowHeight(4, 5) * boxartCellScale);
	//mResultDesc->setSize(mDescContainer->getSize().x(), 0); // make desc text wrap at edge of container

  // Resize Labels
  /*mLabelDeveloper->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mLabelDeveloper->getFont()->Height());
  mLabelPublisher->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mLabelPublisher->getFont()->Height());
  mLabelGenre->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mLabelGenre->getFont()->Height());
  mLabelRating->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mLabelRating->getFont()->Height());
  mLabelReleaseDate->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mLabelReleaseDate->getFont()->Height());
  mLabelPlayers->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mLabelPlayers->getFont()->Height());*/

  // Resize Values
	/*mValueDeveloper->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mValueDeveloper->getFont()->Height());
  mValuePublisher->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mValuePublisher->getFont()->Height());
  mValueGenre->setSize(mGrid.getColWidth(3) * boxartCellScale, (float)mValueGenre->getFont()->Height());
  mValueRating->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mValueDeveloper->getFont()->Height() * 0.65f);
  mValueReleaseDate->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mValueDeveloper->getFont()->Height());
  mValuePlayers->setSize(mGrid.getColWidth(5) * boxartCellScale, (float)mValuePublisher->getFont()->Height());*/

  mGrid.onSizeChanged();
  if(!mLowResolution)
  {
    mBusyAnim.setPosition(mGrid.getColWidth(0), mGrid.getRowHeight(0, 6));
    mBusyAnim.setSize(mGrid.getColWidth(1), mGrid.getRowHeight(7));
  }
}

void ScraperSearchComponent::UpdateInfoPane(const FileData* game)
{
	if (game != nullptr)
	{
		mResultName->setText(game->Name().ToUpperCaseUTF8());
		mResultDesc->SetText(game->Metadata().Description());
		//mDescContainer->reset();

		// Image
        mResultThumbnail->setImage(game->AnyMedia(), false);

        // metadata
		mValueRating->setValue(game->Metadata().Rating());
		mValueReleaseDate->setValue(game->Metadata().ReleaseDateAsString().UpperCase());
		mValueDeveloper->setText(game->Metadata().Developer().UpperCaseUTF8());
		mValuePublisher->setText(game->Metadata().Publisher().UpperCaseUTF8());
		mValueGenre->setText(game->Metadata().Genre().UpperCaseUTF8());
		mValuePlayers->setText(game->Metadata().PlayersAsString().UpperCase());
		mGrid.onSizeChanged();
	}
	else
	{
		mResultName->setText("");
		mResultDesc->SetText("");

    // Image
    mResultThumbnail->setImage(Path(), false);

    // metadata
		mValueRating->setValue("");
		mValueReleaseDate->setValue("");
		mValueDeveloper->setText("");
		mValuePublisher->setText("");
		mValueGenre->setText("");
		mValuePlayers->setText("");
	}
}

bool ScraperSearchComponent::ProcessInput(const InputCompactEvent& event)
{
	return Component::ProcessInput(event);
}

void ScraperSearchComponent::Render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = parentTrans * getTransform();

	renderChildren(trans);

	if (mRunning)
	{
		Renderer::SetMatrix(trans);
		Renderer::DrawRectangle(0.f, 0.f, mSize.x(), mSize.y(), 0x00000011);

    if(!mLowResolution) mBusyAnim.Render(trans);
	}
}

void ScraperSearchComponent::Update(int deltaTime)
{
  Component::Update(deltaTime);

	if (mRunning && !mLowResolution)
    mBusyAnim.Update(deltaTime);
}

bool ScraperSearchComponent::CollectHelpItems(Help& help)
{
  mGrid.CollectHelpItems(help);
	return true;
}
