//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <music/PlaylistCursor.h>

#include <set>

// Where the patron playlist is read from.
//
// The bug this file protects against: the reading head only looked at the two entries
// under it. A box measured on 28/08/2026 had 353 tracks in its playlist, 45 of them fully
// downloaded, and its saved head on entry 111 — with nothing cached between 111 and 150.
// Two entries were enough to give up, the frontend fell back on the theme musics, and
// "Recalbox Main Theme 06" played while 45 patron tracks sat on the disk.
//
// The rule: as long as one entry of the playlist can be played, the cursor hands it out.

static PlaylistCursor CursorOn(int count, int index)
{
  PlaylistCursor cursor;
  cursor.Reset(count, index);
  return cursor;
}

//! Playable set built from explicit indexes
static auto Playable(std::set<int> indexes)
{
  return [indexes](int index) { return indexes.find(index) != indexes.end(); };
}

TEST(PlaylistCursorTest, HandsOutTheEntryUnderTheHeadWhenItCanBePlayed)
{
  PlaylistCursor cursor = CursorOn(10, 4);
  EXPECT_EQ(4, cursor.Seek(Playable({ 2, 4, 7 })));
  EXPECT_EQ(4, cursor.Index());
}

TEST(PlaylistCursorTest, LooksPastMoreThanTwoEntriesThatAreNotDownloadedYet)
{
  // The exact shape of the defect: the third entry was already out of reach.
  PlaylistCursor cursor = CursorOn(10, 0);
  EXPECT_EQ(3, cursor.Seek(Playable({ 3 })));
}

TEST(PlaylistCursorTest, ScansTheWholePlaylistBeforeAdmittingThereIsNothingToPlay)
{
  PlaylistCursor cursor = CursorOn(400, 0);
  EXPECT_EQ(399, cursor.Seek(Playable({ 399 })));
}

TEST(PlaylistCursorTest, WrapsAroundToReachAnEntrySittingBeforeTheHead)
{
  PlaylistCursor cursor = CursorOn(10, 7);
  EXPECT_EQ(2, cursor.Seek(Playable({ 2 })));
  EXPECT_EQ(2, cursor.Index());
}

TEST(PlaylistCursorTest, MovesTheHeadOntoTheEntryItHandedOutAndNotPastIt)
{
  // Whoever plays the track calls Advance() afterwards: the head must still be on it
  // in the meantime, otherwise the track that gets remembered is not the one playing.
  PlaylistCursor cursor = CursorOn(10, 0);
  EXPECT_EQ(6, cursor.Seek(Playable({ 6 })));
  EXPECT_EQ(6, cursor.Index());
}

TEST(PlaylistCursorTest, LeavesTheHeadWhereItIsWhenNothingCanBePlayed)
{
  // The head is what the download loop starts from. Sliding it forward on every failed
  // look made the player run away from the downloader, and the cache ended up scattered
  // over the whole playlist instead of holding the ten tracks that were about to play.
  PlaylistCursor cursor = CursorOn(10, 5);
  EXPECT_EQ(PlaylistCursor::sNoTrack, cursor.Seek(Playable({})));
  EXPECT_EQ(5, cursor.Index());
  EXPECT_EQ(PlaylistCursor::sNoTrack, cursor.Seek(Playable({})));
  EXPECT_EQ(5, cursor.Index());
}

TEST(PlaylistCursorTest, ReportsNoTrackOnAnEmptyPlaylistInsteadOfReadingEntryZero)
{
  PlaylistCursor cursor = CursorOn(0, 0);
  EXPECT_EQ(PlaylistCursor::sNoTrack, cursor.Seek(Playable({ 0 })));
  EXPECT_EQ(PlaylistCursor::sNoTrack, cursor.Index());
}

TEST(PlaylistCursorTest, StartsOverWhenTheSavedPositionIsOutsideTheNewPlaylist)
{
  // The head is restored from a file written for a playlist that may have shrunk since.
  EXPECT_EQ(0, CursorOn(10, 42).Index());
  EXPECT_EQ(0, CursorOn(10, -1).Index());
}

TEST(PlaylistCursorTest, AdvanceStepsToTheEntryFollowingTheOneJustStarted)
{
  PlaylistCursor cursor = CursorOn(10, 3);
  cursor.Advance();
  EXPECT_EQ(4, cursor.Index());
}

TEST(PlaylistCursorTest, AdvanceGoesBackToTheStartAfterTheLastEntry)
{
  PlaylistCursor cursor = CursorOn(10, 9);
  cursor.Advance();
  EXPECT_EQ(0, cursor.Index());
}

TEST(PlaylistCursorTest, AdvanceOnAnEmptyPlaylistKeepsTheHeadHarmless)
{
  PlaylistCursor cursor = CursorOn(0, 0);
  cursor.Advance();
  EXPECT_EQ(PlaylistCursor::sNoTrack, cursor.Index());
}

TEST(PlaylistCursorTest, FindsTheDownloadedTrackOfTheBoxThatWasPlayingTheDefaultMusics)
{
  // Verbatim state read on the box on 28/08/2026: 353 tracks, head restored on entry 111
  // from /overlay/.music/.index, and these 45 entries fully downloaded in
  // /overlay/.music/.cache. The frontend was playing a theme music at that very moment.
  std::set<int> downloaded = { 5, 24, 28, 34, 36, 42, 43, 51, 61, 64, 65, 67, 69, 75, 91,
                               106, 109, 110, 151, 159, 164, 169, 179, 205, 208, 210, 213,
                               215, 230, 236, 239, 248, 255, 258, 270, 276, 287, 295, 296,
                               300, 304, 309, 320, 329, 340 };
  PlaylistCursor cursor = CursorOn(353, 111);
  EXPECT_EQ(151, cursor.Seek(Playable(downloaded)));
}

TEST(PlaylistCursorTest, SkipsTheTracksTheListenerTurnedOff)
{
  // A disabled track is not playable: a run of them must not send the frontend
  // back to the theme musics either.
  PlaylistCursor cursor = CursorOn(10, 0);
  std::set<int> downloaded = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  std::set<int> disabled = { 0, 1, 2, 3, 4, 5 };
  int index = cursor.Seek([&](int i)
    { return downloaded.find(i) != downloaded.end() && disabled.find(i) == disabled.end(); });
  EXPECT_EQ(6, index);
}
