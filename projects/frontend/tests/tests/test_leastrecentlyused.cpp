//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <utils/storage/LeastRecentlyUsed.h>

// Which cached music file gets dropped to make room for the next one.
//
// The bug this file protects against: the election started from a reading of the clock and
// only kept files strictly older than it. File times come from stat() and are UTC seconds,
// while the clock it was compared to was the local wall time read as if it were UTC. On a
// box set to UTC-4, every file downloaded in the last four hours looked like it was in the
// future, no candidate was ever elected, the cache could never be trimmed, and the
// downloader gave up before fetching anything — which is how a box ends up with a full
// cache, a playlist of 353 tracks and the Recalbox theme musics playing.
//
// The rule: the oldest entry is elected among the entries themselves, never against a clock.

TEST(LeastRecentlyUsedTest, ElectsNothingBeforeAnyEntryIsSubmitted)
{
  LeastRecentlyUsed oldest;
  EXPECT_FALSE(oldest.Found());
  EXPECT_EQ(LeastRecentlyUsed::sNothing, oldest.Index());
}

TEST(LeastRecentlyUsedTest, ElectsTheOnlyEntryItWasGiven)
{
  LeastRecentlyUsed oldest;
  oldest.Consider(7, 1000);
  EXPECT_TRUE(oldest.Found());
  EXPECT_EQ(7, oldest.Index());
  EXPECT_EQ(1000, oldest.Timestamp());
}

TEST(LeastRecentlyUsedTest, ElectsTheSmallestTimestampWhateverTheSubmissionOrder)
{
  LeastRecentlyUsed ascending;
  for (int i = 0; i < 5; ++i) ascending.Consider(i, 100 + i);
  EXPECT_EQ(0, ascending.Index());

  LeastRecentlyUsed descending;
  for (int i = 4; i >= 0; --i) descending.Consider(i, 100 + i);
  EXPECT_EQ(0, descending.Index());
}

TEST(LeastRecentlyUsedTest, KeepsTheFirstOfTwoEntriesSharingTheSameTimestamp)
{
  LeastRecentlyUsed oldest;
  oldest.Consider(3, 500);
  oldest.Consider(8, 500);
  EXPECT_EQ(3, oldest.Index());
}

TEST(LeastRecentlyUsedTest, StillElectsAnEntryWhenEveryTimestampSitsInTheFuture)
{
  // The exact situation on a box west of Greenwich: every cached file freshly downloaded,
  // and the value they were compared against four hours behind them.
  LeastRecentlyUsed oldest;
  oldest.Consider(0, 4102444800LL);
  oldest.Consider(1, 4102444700LL);
  oldest.Consider(2, 4102444900LL);
  EXPECT_TRUE(oldest.Found());
  EXPECT_EQ(1, oldest.Index());
}

TEST(LeastRecentlyUsedTest, ElectsAnEntryEvenWhenEveryTimestampIsZero)
{
  // stat() failures report an epoch of zero: they must not make the cache untrimmable.
  LeastRecentlyUsed oldest;
  oldest.Consider(0, 0);
  oldest.Consider(1, 0);
  EXPECT_TRUE(oldest.Found());
  EXPECT_EQ(0, oldest.Index());
}
