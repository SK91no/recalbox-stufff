//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <emulators/run/SaveStateData.h>

TEST(SaveStateDataTest, NoSlotIsAskedForUntilThePlayerPicksOne)
{
  SaveStateData data;
  EXPECT_TRUE(data.SlotNumber().empty());
}

TEST(SaveStateDataTest, TheSlotThePlayerPickedIsCarriedAsIs)
{
  SaveStateData data;
  data.SetSlotNumber(3);
  EXPECT_EQ("3", data.SlotNumber());
}

TEST(SaveStateDataTest, SlotZeroIsASlotOfItsOwnAndNotAnAbsentOne)
{
  SaveStateData data;
  data.SetSlotNumber(0);
  EXPECT_EQ("0", data.SlotNumber());
}

TEST(SaveStateDataTest, StartingWithoutAStateAsksForNoSlotAtAll)
{
  SaveStateData data;
  data.SetSlotNumber(-1);
  EXPECT_TRUE(data.SlotNumber().empty());
}
