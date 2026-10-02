//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <input/InputMapper.h>

//! Two arcade encoders a Recalbox cannot tell apart: same name, same guid, no serial
class TwinPadTest: public ::testing::Test
{
  protected:
    static constexpr const char* sName = "DragonRise Inc.   Generic   USB  Joystick";
    static constexpr const char* sGuid = "0300457e790000000600000010010000";
    static constexpr const char* sLeftPort = "platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0";
    static constexpr const char* sRightPort = "platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.3:1.0";

    static InputMapper::Pad Twin(const char* port, int index)
    {
      return { sName, sGuid, port, Path("/dev/input/event0"), index };
    }
};

TEST_F(TwinPadTest, TwinsCannotBeToldApartByNameAndGuid)
{
  ASSERT_TRUE(Twin(sLeftPort, 0).Same(Twin(sRightPort, 1)));
}

TEST_F(TwinPadTest, TheirPortIsTheOnlyThingThatTellsThemApart)
{
  ASSERT_FALSE(Twin(sLeftPort, 0).SamePort(Twin(sRightPort, 1)));
  ASSERT_TRUE(Twin(sLeftPort, 0).SamePort(Twin(sLeftPort, 1)));
}

TEST_F(TwinPadTest, APadKeepsItsSlotWhateverIndexTheEnumerationGaveIt)
{
  ASSERT_TRUE(Twin(sLeftPort, 0).SamePort(Twin(sLeftPort, 7)));
}

TEST_F(TwinPadTest, APadWithoutAKnownPortNeverClaimsAPortBoundSlot)
{
  ASSERT_FALSE(Twin("", 0).SamePort(Twin("", 1)));
  ASSERT_TRUE(Twin("", 0).Same(Twin("", 1)));
}

TEST_F(TwinPadTest, ADifferentModelOnTheSamePortStaysADifferentPad)
{
  InputMapper::Pad other("Other pad", "0300000000000000000000000000dead", sLeftPort,
                         Path("/dev/input/event1"), 1);
  ASSERT_FALSE(Twin(sLeftPort, 0).SamePort(other));
  ASSERT_FALSE(Twin(sLeftPort, 0).Same(other));
}

TEST_F(TwinPadTest, CopyingAPadCarriesItsPortAlong)
{
  InputMapper::Pad target;
  target.Copy(Twin(sRightPort, 3));
  ASSERT_TRUE(target.SamePort(Twin(sRightPort, 4)));
}

TEST_F(TwinPadTest, ResettingAPadForgetsItsPort)
{
  InputMapper::Pad pad = Twin(sLeftPort, 0);
  pad.Reset();
  ASSERT_FALSE(pad.SamePort(Twin(sLeftPort, 1)));
}
