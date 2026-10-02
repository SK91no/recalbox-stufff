#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-function-cognitive-complexity"
//
// Created by bkg2k on 18/07/22.
//
// As part of the RECALBOX Project
// http://www.recalbox.com
//
#define TEST_ACCESS

#include <gtest/gtest.h>
#include <utils/SectionFile.h>
#include <utils/Files.h>

class SectionFileTest: public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    static Path GetTempFile()
    {
      return Path("/tmp/tmp-test.ini");
    }

    static Path CreateTempFile(const String content)
    {
      Path result = GetTempFile();
      Files::SaveFile(result, content);
      return result;
    }

    static const String SampleFile;
};

const String SectionFileTest::SampleFile = String::Join(
  {
    "# This is the default section",
    " ",
    "Anime1 = Albator",
    "Anime2 = Goldorak",
    "Anime3 = CaptainFlam",
    "Anime.Count = 3",
    "\t",
    "[Albator]",
    "",
    "Character.Count=7",
    "Rating = 19.99",
    "HasNonHuman = false",
    "",
    "[Goldorak]",
    "",
    "Character.Count=12",
    "Rating = 18.75",
    "HasNonHuman = false",
    "",
    "[CaptainFlam]",
    "",
    "Character.Count=9",
    "Rating = 16.53",
    "HasNonHuman = true",
  }
  , "\r\n");

TEST_F(SectionFileTest, TestSectionIdentification)
{
  String name;
  String s("[section.name]");
  ASSERT_TRUE(SectionFile::Section::ExtractSection(s, name));
  ASSERT_TRUE(name == "section.name");
  s.Assign("\t   [section_name]   \t");
  ASSERT_TRUE(SectionFile::Section::ExtractSection(s, name));
  ASSERT_TRUE(name == "section_name");
  s.Assign("\t   [   section-name    ]   \t");
  ASSERT_TRUE(SectionFile::Section::ExtractSection(s, name));
  ASSERT_TRUE(name == "section-name");

  s.Assign("[section");
  ASSERT_FALSE(SectionFile::Section::ExtractSection(s, name));
  s.Assign("section]");
  ASSERT_FALSE(SectionFile::Section::ExtractSection(s, name));
  s.Assign("[section!]");
  ASSERT_FALSE(SectionFile::Section::ExtractSection(s, name));
  s.Assign("[section] # wrong comment");
  ASSERT_FALSE(SectionFile::Section::ExtractSection(s, name));
}

TEST_F(SectionFileTest, TestlineParsing)
{
  String left, right;
  ASSERT_TRUE(SectionFile::Section::ParseLine(String(" \t \t    "), left, right) == SectionFile::LineType::Blank);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("\010 \t \t \x08  "), left, right) == SectionFile::LineType::Binary);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String('#', 5000), left, right) == SectionFile::LineType::TooLong);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("# This is a comment line"), left, right) == SectionFile::LineType::Comment);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("  # This is another comment line"), left, right) == SectionFile::LineType::Comment);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("This is an unreconized free texte line!"), left, right) == SectionFile::LineType::Unidentified);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("\t[section.header-name] "), left, right) == SectionFile::LineType::SectionHeader);
  ASSERT_TRUE(SectionFile::Section::ParseLine(String(";albator=goldorak"), left, right) == SectionFile::LineType::InactiveKeyValue);
  ASSERT_TRUE(left == "albator");
  ASSERT_TRUE(right == "goldorak");
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("  ;chocolatine  =    pain au chocolat\t"), left, right) == SectionFile::LineType::InactiveKeyValue);
  ASSERT_TRUE(left == "chocolatine");
  ASSERT_TRUE(right == "pain au chocolat");
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("kay=velue"), left, right) == SectionFile::LineType::ActiveKeyValue);
  ASSERT_TRUE(left == "kay");
  ASSERT_TRUE(right == "velue");
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("\t key2  \t=\t   value2\t"), left, right) == SectionFile::LineType::ActiveKeyValue);
  ASSERT_TRUE(left == "key2");
  ASSERT_TRUE(right == "value2");
  ASSERT_TRUE(SectionFile::Section::ParseLine(String("k*e!y=value"), left, right) == SectionFile::LineType::InvalidKey);
}

TEST_F(SectionFileTest, TestTypeGetters)
{
  SectionFile sf;
  sf.mDefault.mKeyValues = {
    { "key1", "tagada" },
    { "key2", "1234" },
    { "key3", "0x12345678ABCDEF00" },
    { "key4", "5.123" },
    { "key5", "1" },
    { "key6", "true" },
  };

  ASSERT_TRUE(sf.AsString("key1") == "tagada");
  ASSERT_TRUE(sf.AsString("key2") == "1234");
  ASSERT_TRUE(sf.AsString("key3") == "0x12345678ABCDEF00");
  ASSERT_TRUE(sf.AsString("key4") == "5.123");
  ASSERT_TRUE(sf.AsString("key5") == "1");
  ASSERT_TRUE(sf.AsString("key6") == "true");

  ASSERT_TRUE(sf.AsBool("key1") == false);
  ASSERT_TRUE(sf.AsBool("key2") == false);
  ASSERT_TRUE(sf.AsBool("key3") == false);
  ASSERT_TRUE(sf.AsBool("key4") == false);
  ASSERT_TRUE(sf.AsBool("key5") == true);
  ASSERT_TRUE(sf.AsBool("key6") == true);

  ASSERT_TRUE(sf.AsInt("key1") == 0);
  ASSERT_TRUE(sf.AsInt("key2") == 1234);
  ASSERT_TRUE(sf.AsInt("key3") == (int)0xABCDEF00);
  ASSERT_TRUE(sf.AsInt("key4") == 0);
  ASSERT_TRUE(sf.AsInt("key5") == 1);
  ASSERT_TRUE(sf.AsInt("key6") == 0);

  ASSERT_TRUE(sf.AsInt64("key1") == 0LL);
  ASSERT_TRUE(sf.AsInt64("key2") == 1234LL);
  ASSERT_TRUE(sf.AsInt64("key3") == 0x12345678ABCDEF00LL);
  ASSERT_TRUE(sf.AsInt64("key4") == 0LL);
  ASSERT_TRUE(sf.AsInt64("key5") == 1LL);
  ASSERT_TRUE(sf.AsInt64("key6") == 0LL);

  ASSERT_TRUE(sf.AsFloat("key1") == 0.f);
  ASSERT_TRUE(sf.AsFloat("key2") == 1234.f);
  ASSERT_TRUE(sf.AsFloat("key3") == 0.f);
  ASSERT_TRUE(sf.AsFloat("key4") == 5.123f);
  ASSERT_TRUE(sf.AsFloat("key5") == 1.f);
  ASSERT_TRUE(sf.AsFloat("key6") == 0.f);

  ASSERT_TRUE(sf.AsDouble("key1") == 0.);
  ASSERT_TRUE(sf.AsDouble("key2") == 1234.);
  ASSERT_TRUE(sf.AsDouble("key3") == 0.);
  ASSERT_TRUE(sf.AsDouble("key4") == 5.123);
  ASSERT_TRUE(sf.AsDouble("key5") == 1.);
  ASSERT_TRUE(sf.AsDouble("key6") == 0.);
}

TEST_F(SectionFileTest, TestDefaultGettersAndSetters)
{
  SectionFile sf;
  sf.SetAsString("key1", "tagada");
  sf.SetAsInt("key2", 1234);
  sf.SetAsInt64("key3", 9876543210123);
  sf.SetAsFloat("key4", 9.852f);
  sf.SetAsDouble("key5", 123456.789012);
  sf.SetAsFloat("key6", 9.852f, 3);
  sf.SetAsDouble("key7", 123456.78905, 5);
  sf.SetAsBool("key8", true);
  sf.SetAsBoolText("key9", true);

  ASSERT_TRUE(sf.AsString("key1") == "tagada");
  ASSERT_TRUE(sf.AsString("key2") == "1234");
  ASSERT_TRUE(sf.AsString("key3") == "9876543210123");
  ASSERT_TRUE(sf.AsString("key4") == "9.85");
  ASSERT_TRUE(sf.AsString("key5") == "123456.79");
  ASSERT_TRUE(sf.AsString("key6") == "9.852");
  ASSERT_TRUE(sf.AsString("key7") == "123456.78905");
  ASSERT_TRUE(sf.AsString("key8") == "1");
  ASSERT_TRUE(sf.AsString("key9") == "true");
}

TEST_F(SectionFileTest, TestSectionGettersAndSetters)
{
  SectionFile sf;
  sf.SetAsString("StringAndInt", "key1", "tagada");
  sf.SetAsInt("StringAndInt", "key2", 1234);
  sf.SetAsInt64("StringAndInt", "key3", 9876543210123);
  sf.SetAsFloat("Floats", "key1", 9.852f);
  sf.SetAsDouble("Floats", "key2", 123456.789012);
  sf.SetAsFloat("Doubles", "key1", 9.852f, 3);
  sf.SetAsDouble("Doubles", "key2", 123456.78905, 5);
  sf.SetAsBool("Booleans", "key1", true);
  sf.SetAsBoolText("Booleans", "key2", true);

  ASSERT_TRUE(sf.AsString("StringAndInt", "key1") == "tagada");
  ASSERT_TRUE(sf.AsString("StringAndInt", "key2") == "1234");
  ASSERT_TRUE(sf.AsString("StringAndInt", "key3") == "9876543210123");
  ASSERT_TRUE(sf.AsString("Floats", "key1") == "9.85");
  ASSERT_TRUE(sf.AsString("Floats", "key2") == "123456.79");
  ASSERT_TRUE(sf.AsString("Doubles", "key1") == "9.852");
  ASSERT_TRUE(sf.AsString("Doubles", "key2") == "123456.78905");
  ASSERT_TRUE(sf.AsString("Booleans", "key1") == "1");
  ASSERT_TRUE(sf.AsString("Booleans", "key2") == "true");
}

TEST_F(SectionFileTest, TestCheckImport)
{
  SectionFile sf;
  sf.Import(String(SampleFile).Remove('\r').Split('\n'), false);

  ASSERT_TRUE(sf.AsInt("Anime.Count") == 3);
  ASSERT_TRUE(sf.AsString("Anime1") == "Albator");
  ASSERT_TRUE(sf.AsString("Anime2") == "Goldorak");
  ASSERT_TRUE(sf.AsString("Anime3") == "CaptainFlam");

  ASSERT_TRUE(sf.AsInt("Albator", "Character.Count") == 7);
  ASSERT_TRUE(sf.AsFloat("Albator", "Rating") == 19.99f);
  ASSERT_TRUE(sf.AsBool("Albator", "HasNonHuman") == false);

  ASSERT_TRUE(sf.AsInt("Goldorak", "Character.Count") == 12);
  ASSERT_TRUE(sf.AsFloat("Goldorak", "Rating") == 18.75f);
  ASSERT_TRUE(sf.AsBool("Goldorak", "HasNonHuman") == false);

  ASSERT_TRUE(sf.AsInt("CaptainFlam", "Character.Count") == 9);
  ASSERT_TRUE(sf.AsDouble("CaptainFlam", "Rating") == 16.53);
  ASSERT_TRUE(sf.AsBool("CaptainFlam", "HasNonHuman") == true);
}

TEST_F(SectionFileTest, TestCheckExport)
{
  SectionFile sf;
  sf.SetExtraSpaceAroundSeparator(true);

  sf.SetAsInt("Anime.Count", 3);
  sf.SetAsString("Anime1", "Albator");
  sf.SetAsString("Anime2", "Goldorak");
  sf.SetAsString("Anime3", "CaptainFlam");

  sf.SetAsInt("Albator", "Character.Count", 7);
  sf.SetAsFloat("Albator", "Rating", 19.5f);
  sf.SetAsBool("Albator", "HasNonHuman", false);

  sf.SetAsInt("Goldorak", "Character.Count", 12);
  sf.SetAsFloat("Goldorak", "Rating", 18.75f);
  sf.SetAsBool("Goldorak", "HasNonHuman", false);

  sf.SetAsInt("CaptainFlam", "Character.Count", 9);
  sf.SetAsDouble("CaptainFlam", "Rating", 16.53);
  sf.SetAsBool("CaptainFlam", "HasNonHuman", true);

  String s = sf.ExportToString();
  String::List verify = s.Split('[');
  ASSERT_TRUE(verify.size() == 4);
  ASSERT_TRUE(verify[0].Contains("Anime.Count = 3"));
  ASSERT_TRUE(verify[0].Contains("Anime1 = Albator"));
  ASSERT_TRUE(verify[0].Contains("Anime2 = Goldorak"));
  ASSERT_TRUE(verify[0].Contains("Anime3 = CaptainFlam"));

  bool albator = false, goldorak = false, captainflam = false;
  for(int i = (int)verify.size(); --i > 0;)
  {
    const String& v = verify[i];
    if (v.StartsWith("Albator") && !albator)
    {
      ASSERT_TRUE(v.Contains("Character.Count = 7"));
      ASSERT_TRUE(v.Contains("Rating = 19.5"));
      ASSERT_TRUE(v.Contains("HasNonHuman = 0"));
      albator = true;
    }
    else if (v.StartsWith("Goldorak") && !goldorak)
    {
      ASSERT_TRUE(v.Contains("Character.Count = 12"));
      ASSERT_TRUE(v.Contains("Rating = 18.75"));
      ASSERT_TRUE(v.Contains("HasNonHuman = 0"));
      goldorak = true;
    }
    else if (v.StartsWith("CaptainFlam") && !captainflam)
    {
      ASSERT_TRUE(v.Contains("Character.Count = 9"));
      ASSERT_TRUE(v.Contains("Rating = 16.53"));
      ASSERT_TRUE(v.Contains("HasNonHuman = 1"));
      captainflam = true;
    }
    else ASSERT_TRUE(false);
  }
}

TEST_F(SectionFileTest, TestImportModifyExport)
{
  SectionFile sf(CreateTempFile(SampleFile), true);

  sf.SetAsString("Anime1", "Albator - Le Corsaire de l'Espace");

  sf.SetAsInt("Albator", "Character.Count", 8);
  sf.SetAsBoolText("Goldorak", "HasNonHuman", true);
  sf.SetAsDouble("CaptainFlam", "Rating", 15.5);

  String s = sf.ExportToString();
  String::List verify = s.Split('[');
  ASSERT_TRUE(verify.size() == 4);
  ASSERT_TRUE(verify[0].Contains("Anime.Count = 3"));
  ASSERT_TRUE(verify[0].Contains("Anime1 = Albator"));
  ASSERT_TRUE(verify[0].Contains("Anime2 = Goldorak"));
  ASSERT_TRUE(verify[0].Contains("Anime3 = CaptainFlam"));

  bool albator = false, goldorak = false, captainflam = false;
  for(int i = (int)verify.size(); --i > 0;)
  {
    const String& v = verify[i];
    if (v.StartsWith("Albator") && !albator)
    {
      ASSERT_TRUE(v.Contains("Character.Count = 8"));
      ASSERT_TRUE(v.Contains("Rating = 19.99"));
      ASSERT_TRUE(v.Contains("HasNonHuman = false"));
      albator = true;
    }
    else if (v.StartsWith("Goldorak") && !goldorak)
    {
      ASSERT_TRUE(v.Contains("Character.Count=12"));
      ASSERT_TRUE(v.Contains("Rating = 18.75"));
      ASSERT_TRUE(v.Contains("HasNonHuman = true"));
      goldorak = true;
    }
    else if (v.StartsWith("CaptainFlam") && !captainflam)
    {
      ASSERT_TRUE(v.Contains("Character.Count=9"));
      ASSERT_TRUE(v.Contains("Rating = 15.5"));
      ASSERT_TRUE(v.Contains("HasNonHuman = true"));
      captainflam = true;
    }
    else ASSERT_TRUE(false);
  }
}

#pragma clang diagnostic pop
