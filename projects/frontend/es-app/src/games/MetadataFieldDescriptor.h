#pragma once

#include <utils/String.h>
#include <utils/Xml.h>
#include <utils/os/fs/Path.h>
#include <games/MetadataType.h>

//! Method pointer to default value getters
typedef bool (MetadataDescriptor::*IsDefaultValueMethodType)() const;
//! Method pointer to string getters
typedef String (MetadataDescriptor::*GetValueMethodType)() const;
//! Method pointer to string setters
typedef void (MetadataDescriptor::*SetValueMethodType)(const String& value);
//! Method pointer to string setters
typedef void (MetadataDescriptor::*SetPathMethodType)(const Path& value);
//! Method pointer to XML setters (takes priority over SetValueMethodType when not null)
//! Both receive the game/folder node itself: the field owns its whole subtree, including its node name.
typedef void (MetadataDescriptor::*SetXmlValueMethodType)(const XmlNode& parent, const Path& relativeTo);
//! Method pointer to XML serializers (takes priority over GetValueMethodType when not null)
typedef void (MetadataDescriptor::*SerializeXmlMethodType)(XmlNode& parent, const Path& relativeTo) const;

class MetadataFieldDescriptor
{
  public:
    //! Edition type
    enum class EditableType
    {
        None,      //! Not editable
        List,      //!< Single-selection list
        ListMulti, //!< Multi-selection list
        Text,      //!< Simple text edit
        Switch,    //!< Switch on/off
        Date,      //!< Date picker
        Rating,    //!< Special rating
    };

    //! Data type
    enum class DataType
    {
        // Simple types
        String,  //!< String
        Int,     //!< int
        Short,   //!< short
        Bool,    //!< bool
        Float,   //!< float

        // Derived types
        Text,    //!< Multiline text (String)
        List,    //!< Simple text from a fixed list
        Path,    //!< File path (String)
        Rating,  //!< Floating point value between 0.0 and 1.0 (float)
        Date,    //!< Epoc (int)
        Range,   //!< Integer range: LSW:from MSW:to
        Crc32,   //!< 4byte hash (int)
    };

  private:
    String                    _Key;                  //!< Identifier
    String                    _DefaultValue;         //!< default value
    MetadataType              _MetadataType;         //!< Named metadata type
    DataType                  _DataType;             //!< Datatype
    EditableType              _EditType;             //!< Editable type
    IsDefaultValueMethodType  _IsDefaultValueMethod; //!< Is Default value?
    GetValueMethodType        _GetMethod;            //!< String getter
    SerializeXmlMethodType    _SerializeXmlMethod;   //!< XML serializer (when not null, takes priority over _GetMethod)
    SetValueMethodType        _SetMethod;            //!< String getter
    SetXmlValueMethodType     _SetXmlMethod;         //!< XML setter (when not null, takes priority over _SetMethod)
    bool                      _IsStatistic;          //!< if true, ignore scraper values for this metadata
    bool                      _IsMain;               //!< if true, display on main metadata editor GUI, else in secondary
    bool                      _IsUserData;           //!< If true, data goes into the userdata gamelist, not in the main gamelist

  public:
    // Public const accessors
    [[nodiscard]] const String&             Key()                  const { return _Key;                  } //!< Identifier
    [[nodiscard]] const String&             DefaultValue()         const { return _DefaultValue;         } //!< default value
    [[nodiscard]] MetadataType              MetaType()             const { return _MetadataType;         } //!< Named metadata type
    [[nodiscard]] DataType                  Type()                 const { return _DataType;             } //!< Datatype
    [[nodiscard]] EditableType              EditType()             const { return _EditType;             } //!< Editable type
    [[nodiscard]] IsDefaultValueMethodType  IsDefaultValueMethod() const { return _IsDefaultValueMethod; } //!< Is Default value?
    [[nodiscard]] GetValueMethodType        GetValueMethod()       const { return _GetMethod;            } //!< String getter
    [[nodiscard]] SerializeXmlMethodType    SerializeXmlMethod()   const { return _SerializeXmlMethod;   } //!< XML serializer (when not null, takes priority over GetValueMethod())
    [[nodiscard]] SetValueMethodType        SetValueMethod()       const { return _SetMethod;            } //!< String setter
    [[nodiscard]] SetXmlValueMethodType     SetXmlValueMethod()    const { return _SetXmlMethod;         } //!< XML setter (when not null, takes priority over SetValueMethod())
    [[nodiscard]] bool                      IsStatistic()          const { return _IsStatistic;          } //!< if true, ignore scraper values for this metadata
    [[nodiscard]] bool                      IsMain()               const { return _IsMain;               } //!< if true, display on main metadata editor GUI, else in secondary
    [[nodiscard]] bool                      IsUserdata()           const { return _IsUserData;           } //!< If true, data goes into the userdata gamelist, not in the main gamelist

    //! Constructor
    MetadataFieldDescriptor(const String&        key,
                            const String&        defaultValue,
                            MetadataType              metadataType,
                            DataType                  type,
                            EditableType              edittype,
                            IsDefaultValueMethodType  isDefaultValueMethod,
                            GetValueMethodType        getMethod,
                            SerializeXmlMethodType    serializeXmlMethod,
                            SetValueMethodType        setMethod,
                            SetXmlValueMethodType     setXmlMethod,
                            bool                      isStatistic,
                            bool                      isMain,
                            bool                      isUserData)
      : _Key(key)
      , _DefaultValue(defaultValue)
      , _MetadataType(metadataType)
      , _DataType(type)
      , _EditType(edittype)
      , _IsDefaultValueMethod(isDefaultValueMethod)
      , _GetMethod(getMethod)
      , _SerializeXmlMethod(serializeXmlMethod)
      , _SetMethod(setMethod)
      , _SetXmlMethod(setXmlMethod)
      , _IsStatistic(isStatistic)
      , _IsMain(isMain)
      , _IsUserData(isUserData)
    {
    }
};

