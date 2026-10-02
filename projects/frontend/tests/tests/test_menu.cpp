//
// Created by bkg2k on 16/04/25.
//

#include <gtest/gtest.h>
#include <guis/menus/MenuDeserializer.h>


class MenuTest: public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(MenuTest, testDeserialize)
{
  MenuDeserializer menu;
  Path path("./data/resources/menu.xml");
  ASSERT_TRUE(path.Exists());
  ASSERT_TRUE(menu.DeserializeXml(path));
}
