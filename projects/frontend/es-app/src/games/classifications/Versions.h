//
// Created by gugue_u on 02/03/2022.
//

#include <utils/String.h>

class Versions
{
  public:
    enum class GameVersions : unsigned int
    {
      Unknown = 0,

      Sample = 1,
      Sample1 = 1,
      Sample2 = 2,

      Proto = 11,
      Proto1 = 11,
      Proto2 = 12,

      Beta = 20,
      Beta1 = 21,
      Beta2 = 22,
      Beta3 = 23,
      Beta4 = 24,
      Beta5 = 25,
      Beta6 = 26,
      Beta7 = 27,
      Beta8 = 28,
      Beta9 = 29,

      Alt  = 50,
      Alt1 = 51,
      Alt2 = 52,
      Alt3 = 53,
      Alt4 = 54,
      Alt5 = 55,
      Alt6 = 56,
      Alt7 = 57,
      Alt8 = 58,
      Alt9 = 58,

      None = 50,

      Rev = 100,
      Rev1 = 101,
      Rev2 = 102,
      Rev3 = 103,
      Rev4 = 104,
      Rev5 = 105,
      Rev6 = 106,
      Rev7 = 107,
      Rev8 = 108,
      Rev9 = 109,

      RevA = 201,
      RevB = 202,
      RevC = 203,
      RevD = 204,
      RevE = 205,
      RevF = 206,
      RevG = 207,
      RevH = 208,

      V1_1 = 301,
      V1_2 = 302,
      V1_3 = 303,
      V1_4 = 304,
      V1_5 = 305,
      V1_6 = 306,
      V1_7 = 307,
      V1_8 = 308,
      V1_9 = 308,

      V2_1 = 401,
      V2_2 = 402,
      V2_3 = 403,
      V2_4 = 404,
      V2_5 = 405,
      V2_6 = 406,
      V2_7 = 407,
      V2_8 = 408,
      V2_9 = 408,

      VirtualConsole = 1000,
    };

    /*!
     * @brief extract game version from no intro game
     * @param File name
     * @return game versions
     */
    static Versions::GameVersions ExtractGameVersionNoIntro(const String& fileName);

  private:
    /*!
     * @brief Deserialize game version
     * @param string tag
     * @return GameVersions
     */
    static Versions::GameVersions Deserialize(const String& tag);

    /*!
     * @brief Deserialize game version
     * @param game version
     * @return game version has string
     */
    static String& Serialize(GameVersions versions);
};


