//
// Created by bkg2k on 16/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#include <gtest/gtest.h>
#include <utils/Sizes.h>

class SizesTest: public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(SizesTest, TestConverters)
{
  long long size = 0x7FFFFFFFFFFFFFFFLL;
  Sizes s(size);
  ASSERT_EQ(s.ToBytes(), "9223372036854775807B");
  ASSERT_EQ(s.ToKiloBytes(), "9007199254740992.00KB");
  ASSERT_EQ(s.ToMegaBytes(), "8796093022208.00MB");
  ASSERT_EQ(s.ToGigaBytes(), "8589934592.00GB");
  ASSERT_EQ(s.ToTeraBytes(), "8388608.00TB");
}

TEST_F(SizesTest, TestConvertToHumanSize)
{
  ASSERT_EQ(Sizes(123LL).ToHumanSize(), "123B");
  ASSERT_EQ(Sizes(0x1EE00LL).ToHumanSize(), "123.50KB");
  ASSERT_EQ(Sizes(0x7B80000LL).ToHumanSize(), "123.50MB");
  ASSERT_EQ(Sizes(0x1EE0000000LL).ToHumanSize(), "123.50GB");
  ASSERT_EQ(Sizes(0x7B8000000000LL).ToHumanSize(), "123.50TB");
}