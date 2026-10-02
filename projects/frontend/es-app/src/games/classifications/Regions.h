//
// Created by Bkg2k on 07/02/2020.
//
#pragma once

#include <utils/os/fs/Path.h>

class Regions
{
  public:
    enum class OrderedMasterRegions
    {
      WorUsEuJp,
      WorUsJpEu,
      WorEuUsJp,
      WorEuJpUs,
      WorJpUsEu,
      WorJpEuUs,
      UsWorEuJp,
      UsWorJpEu,
      UsEuWorJp,
      UsEuJpWor,
      UsJpWorEu,
      UsJpEuWor,
      EuWorUsJp,
      EuWorJpUs,
      EuUsWorJp,
      EuUsJpWor,
      EuJpWorUs,
      EuJpUsWor,
      JpWorUsEu,
      JpWorEuUs,
      JpUsWorEu,
      JpUsEuWor,
      JpEuWorUs,
      JpEuUsWor,
      AsiWorUsEuJp,
      AsiWorUsJpEu,
      AsiWorJpUsEu,
      AsiWorJpEuUs,
      AmeWorUsEuJp,
      AmeWorEuUsJp,
      AmeUsWorEuJp,
      AmeEuWorUsJp,
      __Count,
    };

    enum class GameRegions : unsigned char
    {
      Unknown = 0, // Unknown/unset region
      // Master regions
      WOR, // World
      US, // USA
      EU, // Europe
      JP, // Japan
      ASI, // Asia
      AME, // Latin America
      // Countries or regions
      AD, // Andorra
      AE, // United Arab Emirates
      AF, // Afghanistan
      AG, // Antigua and Barbuda
      AI, // Anguilla
      AL, // Albania
      AM, // Armenia
      AO, // Angola
      AR, // Argentina
      AS, // American Samoa
      AT, // Austria
      AU, // Australia
      AW, // Aruba
      AX, // Åland Islands
      AZ, // Azerbaijan
      BA, // Bosnia and Herzegovina
      BB, // Barbados
      BD, // Bangladesh
      BE, // Belgium
      BF, // Burkina Faso
      BG, // Bulgaria
      BI, // Burundi
      BJ, // Benin
      BL, // Saint Barthélemy
      BM, // Bermuda
      BN, // Brunei
      BQ, // Bonaire, Sint Eustatius and Saba
      BS, // Bahamas
      BT, // Bhutan
      BW, // Botswana
      BY, // Belarus
      BZ, // Belize
      BH, // Bahrain
      BO, // Bolivia
      BR, // Brazil
      CA, // Canada
      CAR, // Caribbean
      CC, // Cocos (Keeling) Islands
      CD, // Congo (DRC)
      CF, // Central African Republic
      CG, // Congo
      CH, // Switzerland
      CI, // Côte d’Ivoire
      CK, // Cook Islands
      CL, // Chile
      CM, // Cameroon
      CN, // China
      CO, // Colombia
      CR, // Costa Rica
      CU, // Cuba
      CV, // Cabo Verde
      CW, // Curaçao
      CX, // Christmas Island
      CY, // Cyprus
      CZ, // Czechia
      DE, // Germany
      DJ, // Djibouti
      DK, // Denmark
      DM, // Dominica
      DO, // Dominican Republic
      DZ, // Algeria
      EC, // Ecuador
      EE, // Estonia
      EG, // Egypt
      ER, // Eritrea
      ES, // Spain
      ET, // Ethiopia
      FI, // Finland
      FJ, // Fiji
      FK, // Falkland Islands
      FM, // Micronesia
      FO, // Faroe Islands
      FR, // France
      GA, // Gabon
      GD, // Grenada
      GE, // Georgia
      GF, // French Guiana
      GG, // Guernsey
      GH, // Ghana
      GI, // Gibraltar
      GL, // Greenland
      GM, // Gambia
      GN, // Guinea
      GP, // Guadeloupe
      GQ, // Equatorial Guinea
      GR, // Greece
      GT, // Guatemala
      GU, // Guam
      GW, // Guinea-Bissau
      GY, // Guyana
      HK, // Hong Kong SAR
      HN, // Honduras
      HR, // Croatia
      HT, // Haiti
      HU, // Hungary
      ID, // Indonesia
      IE, // Ireland
      IL, // Israel
      IM, // Isle of Man
      IN, // India
      IO, // British Indian Ocean Territory
      IQ, // Iraq
      IR, // Iran
      IS, // Iceland
      IT, // Italy
      JE, // Jersey
      JM, // Jamaica
      JO, // Jordan
      KE, // Kenya
      KG, // Kyrgyzstan
      KH, // Cambodia
      KI, // Kiribati
      KM, // Comoros
      KN, // Saint Kitts and Nevis
      KP, // North Korea
      KR, // Korea
      KW, // Kuwait
      KY, // Cayman Islands
      KZ, // Kazakhstan
      LA, // Laos
      LC, // Saint Lucia
      LB, // Lebanon
      LI, // Liechtenstein
      LK, // Sri Lanka
      LR, // Liberia
      LS, // Lesotho
      LT, // Lithuania
      LU, // Luxembourg
      LV, // Latvia
      LY, // Libya
      MA, // Morocco
      MC, // Monaco
      MD, // Moldova
      ME, // Montenegro
      MF, // Saint Martin
      MG, // Madagascar
      MH, // Marshall Islands
      MK, // Macedonia, FYRO
      ML, // Mali
      MM, // Myanmar
      MN, // Mongolia
      MO, // Macao SAR
      MP, // Northern Mariana Islands
      MQ, // Martinique
      MR, // Mauritania
      MS, // Montserrat
      MT, // Malta
      MU, // Mauritius
      MV, // Maldives
      MW, // Malawi
      MX, // Mexico
      MY, // Malaysia
      MZ, // Mozambique
      NA, // Namibia
      NC, // New Caledonia
      NE, // Niger
      NF, // Norfolk Island
      NG, // Nigeria
      NI, // Nicaragua
      NL, // Netherlands
      NO, // Norway
      NP, // Nepal
      NR, // Nauru
      NU, // Niue
      NZ, // New Zealand
      OM, // Oman
      PA, // Panama
      PE, // Peru
      PF, // French Polynesia
      PG, // Papua New Guinea
      PH, // Philippines
      PK, // Pakistan
      PL, // Poland
      PM, // Saint Pierre and Miquelon
      PN, // Pitcairn Islands
      PR, // Puerto Rico
      PS, // Palestinian Authority
      PT, // Portugal
      PW, // Palau
      PY, // Paraguay
      QA, // Qatar
      RE, // Réunion
      RO, // Romania
      RS, // Serbia
      RU, // Russia
      RW, // Rwanda
      SA, // Saudi Arabia
      SB, // Solomon Islands
      SC, // Seychelles
      SD, // Sudan
      SE, // Sweden
      SG, // Singapore
      SH, // St Helena, Ascension, Tristan da Cunha
      SI, // Slovenia
      SJ, // Svalbard and Jan Mayen
      SK, // Slovakia
      SL, // Sierra Leone
      SM, // San Marino
      SN, // Senegal
      SO, // Somalia
      SR, // Suriname
      SS, // South Sudan
      ST, // São Tomé and Príncipe
      SV, // El Salvador
      SX, // Sint Maarten
      SY, // Syria
      SZ, // Swaziland
      TC, // Turks and Caicos Islands
      TD, // Chad
      TG, // Togo
      TH, // Thailand
      TJ, // Tajikistan
      TK, // Tokelau
      TL, // Timor-Leste
      TM, // Turkmenistan
      TN, // Tunisia
      TO, // Tonga
      TR, // Turkey
      TT, // Trinidad and Tobago
      TV, // Tuvalu
      TW, // Taiwan
      TZ, // Tanzania
      UA, // Ukraine
      UG, // Uganda
      UK, // United Kingdom
      UM, // U.S. Outlying Islands
      UY, // Uruguay
      UZ, // Uzbekistan
      VA, // Vatican City
      VC, // Saint Vincent and the Grenadines
      VE, // Venezuela
      VG, // British Virgin Islands
      VI, // U.S. Virgin Islands
      VN, // Vietnam
      VU, // Vanuatu
      WF, // Wallis and Futuna
      WS, // Samoa
      XK, // Kosovo
      YE, // Yemen
      YT, // Mayotte
      ZA, // South Africa
      ZM, // Zambia
      ZW, // Zimbabwe
      __Count
    };

    // Packed regions
    union RegionPack
    {
      int                  Pack;                  //! Packed regions
      static constexpr int sMaxRegions = sizeof(Pack);
      GameRegions          Regions[sMaxRegions]; //! Individual regions

      //! Initializer
      RegionPack() : Pack(0)
      {
      }

      //! Copy constructor
      RegionPack(const RegionPack& source)
      {
        Pack = source.Pack;
      }
      //! = operator
      RegionPack& operator =(const RegionPack& right) { if (this != &right) Pack = right.Pack; return *this; }

      //! Push a new region in an empty slot
      RegionPack& Push(GameRegions region)
      {
        for(GameRegions& slot : Regions)
          if (slot == GameRegions::Unknown)
          {
            slot = region;
            break;
          }
        return *this;
      }

      //! Push a new region pack in empty slots
      RegionPack& Push(const RegionPack& regions)
      {
        for(GameRegions slot : regions.Regions)
          if (slot != GameRegions::Unknown)
            Push(slot);
        return *this;
      }

      //! Sort regions, lowest value in highest slot (MSB of Pack)
      RegionPack& Sort()
      {
        unsigned int pack = (unsigned int)Pack;
        if (pack != 0)
        {
          unsigned char i0 = (unsigned char) pack;
          unsigned char i1 = (unsigned char) (pack >> 8);
          unsigned char i2 = (unsigned char) (pack >> 16);
          unsigned char i3 = (unsigned char) (pack >> 24);
          #define __SwapByte(a, b) { unsigned char c = (b); (b) = (a); (a) = c; }
          if (i0 < i1) __SwapByte(i0, i1)
          if (i2 < i3) __SwapByte(i2, i3)
          if (i0 < i2) __SwapByte(i0, i2)
          if (i1 < i3) __SwapByte(i1, i3)
          if (i1 < i2) __SwapByte(i1, i2)
          #undef __SwapByte
          pack = (int) (i0 | ((unsigned int) i1 << 8) | ((unsigned int) i2 << 16) | ((unsigned int) i3 << 24));
          if ((pack >> 24) == 0)
            if (pack <<= 8; (pack >> 24) == 0)
              if (pack <<= 8; (pack >> 24) == 0)
                pack <<= 8;
          Pack = (int)pack;
        }
        return *this;
      }

      //! Opoerator - for sorting
      int operator -(const RegionPack& right) const
      {
        if (int d = (int)Regions[3] - (int)right.Regions[3]; d != 0) return d;
        if (int d = (int)Regions[2] - (int)right.Regions[2]; d != 0) return d;
        if (int d = (int)Regions[1] - (int)right.Regions[1]; d != 0) return d;
        return (int)Regions[0] - (int)right.Regions[0];
      }

      //! Opoerator ==
      bool operator ==(const RegionPack& right) const { return Pack == right.Pack; }
      //! Opoerator !=
      bool operator !=(const RegionPack& right) const { return Pack != right.Pack; }

      //! Reset
      void Reset() { Pack = 0; }

      //! Has any regions?
      [[nodiscard]] bool HasRegion() const { return Pack != 0; }

      //! Has a particular region?
      [[nodiscard]] bool HasRegion(GameRegions r) const
      {
        for(GameRegions region : Regions)
          if (region == r)
            return true;
        return false;
      }

      //! Get first available region
      [[nodiscard]] GameRegions First() const
      {
        for(GameRegions region : Regions)
          if (region != GameRegions::Unknown)
            return region;
        return GameRegions::Unknown;
      }

      [[nodiscard]] int Count() const
      {
        int count = 0;
        for(GameRegions region : Regions)
          if (region != GameRegions::Unknown)
            count++;
        return count;
      }
    } __attribute((packed));

    //! Region list
    typedef std::vector<GameRegions> List;

    /*!
     * @brief Get full region name (english)
     * @param region Region to get full name from
     * @return Full name
     */
    static const String& RegionFullName(GameRegions region);

    /*!
     * @brief Get region from its full name
     * @param region Region fullname
     * @return GameRegions or Unknown
     */
    static GameRegions FullNameToRegions(const String& region);

    /*!
     * @brief Serialize a single region to its string representation
     * @param region Region to serialize
     * @return String
     */
    static const String& SerializeRegion(GameRegions region);

    /*!
     * @brief Deserialize a string representation of a region
     * @param region String representation
     * @return Region (Unknown if not reconized)
     */
    static GameRegions DeserializeRegion(const String& region);

    /*!
     * @brief Deserialize up to 4 regions from a comma separated string
     * @param regions Regions, comma separated
     * @return Region pack (up to 4)
     */
    static Regions::RegionPack Deserialize4Regions(const String& regions);

    /*!
     * @brief Serialize up to 4 regions into their string representation, comma separated
     * @param regions Compact regions (1 per byte)
     * @return String representation, comma separated
     */
    static String Serialize4Regions(const Regions::RegionPack& regions);

    /*!
     * @brief Extract one region from file name
     * @param path
     * @return
     */
    static Regions::RegionPack ExtractRegionsFromFileName(const Path& path);

    /*!
     * @brief Check and clamp the value inside valid range
     * @param region Region to check
     * @return Clamped region
     */
    static GameRegions Clamp(GameRegions region)
    {
      for(GameRegions aregion : AvailableRegions())
        if (region == aregion)
          return region;
      return GameRegions::Unknown;
    }

    /*!
     * @brief Get list of available regions
     * @return Regions
     */
    static const List& AvailableRegions();

    /*!
     * @brief Check if the given region is in one of the 4 compact regions
     * @return true if region is in regions
     */
    static bool IsIn4Regions(unsigned int regions, GameRegions region);

    /*!
     * @brief Convert aggregated ordered master regions into 4 to 5 regions in the lower bytes of the given int64
     * @param master Ordered master regions
     * @param regions Output variable
     * @return Number of regions in output variable regions
     */
    static int OrderedMasterRegionsTo8Regions(OrderedMasterRegions master, [[out]] long long int& regions);

    /*!
     * @brief Convert ordered master region to a displayable string
     * @param master Ordered master region to convert
     * @return Displayable string
     */
    static const String& OrderedMasterRegionsFromEnum(OrderedMasterRegions master);

    /*!
     * @brief Convert string to ordered master rtegions enum
     * @param master Ordered master region to convert
     * @return Enum or default value
     */
    static Regions::OrderedMasterRegions OrderedMasterRegionsFromString(const String& master);

    static GameRegions GameRegionsFromString(const String& gameRegions);
    static const String& GameRegionsFromEnum(GameRegions gameRegions);

    static Regions::RegionPack ExtractRegionsFromNoIntroName(const String& fileName);
    static Regions::RegionPack ExtractRegionsFromTosecName(const String& fileName);

    static Regions::RegionPack ExtractRegionsFromName(const String& string);

    static Regions::RegionPack ExtractRegionsFromArcadeName(const String& name, bool& nudity);
};

