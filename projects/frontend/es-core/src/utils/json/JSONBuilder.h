/*
 * JSONBuilder.h
 *
 *  Created on: 20 mai 2013
 *      Author: Bkg2k
 */
#pragma once

#include <map>
#include <utils/json/ISerializeToJson.h>
#include <utils/storage/Array.h>
#include <utils/storage/HashMap.h>

/*!
 * A very simple JSON string builder that *exactly* suits our needs.
 * It is not intended to be a complete JSON builder, and yet less a serializer!
 */
class JSONBuilder : public String
{
  private:
    //! Do not insert CRLF when this flag is true
    bool mOptimized;

    /*!
     * Add field separator if required
     */
    void FieldCheck()
    {
      if (!empty())
        if (back() != '{' && back() != '[')
          Append(',');
      if (!mOptimized) Append('\n');
    }

    /*!
     * Add the stringized field name
     * @param name Field name
     * @return Inherited sting instance for method chaining
     */
    JSONBuilder& FieldName(const char* name)
    {
      FieldCheck();
      Append('\"').Append(Escape(name)).Append('\"');
      return *this;
    }

    /*!
     * @brief Escape special chars from the gigen string
     * @param source Source string
     * @return Escaped string
     */
    static String Escape(const String& source)
    {
      #define LENGTHY_STRING(x) x, (int)sizeof(x) - 1
      return String(source)
            .Replace('\\', LENGTHY_STRING("\\\\"))
            .Replace('\b', LENGTHY_STRING("\\b"))
            .Replace('\f', LENGTHY_STRING("\\f"))
            .Replace('\n', LENGTHY_STRING("\\n"))
            .Replace('\r', LENGTHY_STRING("\\r"))
            .Replace('\t', LENGTHY_STRING("\\t"))
            .Replace('\"', LENGTHY_STRING("\\\""));
      #undef LENGTHY_STRING
    }

    /*!
     * @brief Escape special chars from the gigen string
     * @param source Source string
     * @return Escaped string
     */
    static String Escape(const char* source)
    {
      return Escape(String(source));
    }

    /*!
     * @brief Escape special chars from the gigen string
     * @param source Source string
     * @param length Source string length
     * @return Escaped string
     */
    static String Escape(const char* source, int length)
    {
      return Escape(String(source, length));
    }

  public:
    typedef std::vector<JSONBuilder> JSONList;

    /*!
     * Return the inherited string
     * @return String reference
     */
    [[nodiscard]] const String& GetJSONString() const { return *this; }


    explicit JSONBuilder(bool optimized = true)
      : mOptimized(optimized)
    {
    }

    explicit JSONBuilder(int reserved, bool optimized = true)
      : mOptimized(optimized)
    {
      reserve(reserved);
    }

    /*!
     * Open the whole JSON builder
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Open()
    {
      Assign('{');
      return *this;
    }

    /*!
     * Close the builder
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Close()
    {
      Append('}');
      return *this;
    }

    /*!
     * Open a new object
     * @param name Object's name
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& OpenObject(const char* name)
    {
      if (name != nullptr) FieldName(name).Append(':');
      else FieldCheck();
      Append('{');
      return *this;
    }

    /*!
     * Close an object
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& CloseObject() { return Close(); }

    /*!
     * Open a new array
     * @param name Array name
     * @return  The current JSON Instance for method chaining.
     */
    JSONBuilder& OpenArray(const char* name)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      else FieldCheck();
      return *this;
    }

    /*!
     * Close an array
     * @return The current JSON Instance for method chaining
     */
    JSONBuilder& CloseArray()
    {
      Append(']');
      return *this;
    }

    /*!
     * Add a string field
     * @param name Field name
     * @param value Field value
     * @param length Field value length
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const char* value, int length)
    {
      if (name != nullptr) FieldName(name).Append(":\"").Append(Escape(value, length)).Append('"');
      return *this;
    }

    /*!
     * Add a string field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const char* value)
    {
      if (name != nullptr) FieldName(name).Append(":\"").Append(Escape(value)).Append('"');
      return *this;
    }

    /*!
     * Add a string field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const String& value)
    {
      if (name != nullptr) FieldName(name).Append(":\"").Append(Escape(value)).Append('"');
      return *this;
    }

    /*!
     * Add an integer field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, int value)
    {
      if (name != nullptr) FieldName(name).Append(':').Append(value);
      return *this;
    }

    /*!
     * Add a 64-bit integer field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, long long value)
    {
      if (name != nullptr) FieldName(name).Append(':').Append(value);
      return *this;
    }

    /*!
     * Add a float field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, float value)
    {
      if (name != nullptr) FieldName(name).Append(':').Append(value, 2);
      return *this;
    }

    /*!
     * Add a boolean field
     * @param name Field name
     * @param value Field value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, bool value)
    {
      if (name != nullptr) FieldName(name).Append(':').Append(value ? "true" : "false");
      return *this;
    }

    /*!
     * Add a map as a subobject
     * @param name Field name
     * @param value Map
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const HashMap<String, String>& values)
    {
      if (name != nullptr) FieldName(name).Append(":{");
      for(const auto& value : values)
        Field(value.first.c_str(), value.second);
      Append('}');
      return *this;
    }

    /*!
     * Add a string array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const String::List& values)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      for(const String& value : values)
      {
        if (back() != '[') Append(',');
        Append('"').Append(value).Append('"');
      }
      Append(']');
      return *this;
    }

    /*!
     * Add a JSON array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const JSONBuilder::JSONList& values)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      for(const String& value : values)
      {
        if (back() != '[') Append(',');
        Append(value);
      }
      Append(']');
      return *this;
    }

    /*!
     * Add a float array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const float value[], int count)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      for(int i = 0; --count >= 0; ++i)
      {
        Append(value[i], 2);
        if (count != 0) Append(',');
      }
      Append(']');
      return *this;
    }

    /*!
     * Add a long long array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const long long value[], int count)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      for(int i = 0; --count >= 0; i++)
      {
        Append(value[i]);
        if (count != 0) Append(',');
      }
      Append(']');
      return *this;
    }

    /*!
     * Add an int array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const int value[], int count)
    {
      if (name != nullptr) FieldName(name).Append(":[");
      for(int i = 0; --count >= 0; i++)
      {
        Append(value[i]);
        if (count != 0) Append(',');
      }
      Append(']');
      return *this;
    }

    /*!
     * Add a hexadecimal array field
     * @param name Field name
     * @param value Field array
     * @param count value count
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& FieldHexa(const char* name, int value[], int count)
    {
      if (name != nullptr)
      {
        OpenArray(name);
        for(int i = 0; --count >= 0; i++)
        {
          Append('\"').AppendHexa(value[i]).Append('\"');
          if (count != 0) Append(',');
        }
        CloseArray();
      }
      return *this;
    }

    /*!
     * Add ab object from another JSON Builder
     * @param name Field name
     * @param object Object value
     * @return The current JSON Instance for method chaining.
     */
    JSONBuilder& Field(const char* name, const JSONBuilder& object)
    {
      if (name != nullptr) FieldName(name).Append(':').Append(object);
      return *this;
    }

    /*!
     * @brief Serialize a custom object
     * @tparam T Object type
     * @param name JSON Object name
     * @param object Object instance
     * @param serializer Custom serializer implementation
     * @return The current JSON Instance for method chaining.
     */
    template<class T> JSONBuilder& Field(const char* name, const T& object, ISerializeToJson<T>& serializer)
    {
      if (name != nullptr)
      {
        OpenObject(name).Append(serializer.Serialize(&object));
        CloseObject();
      }
      return *this;
    }

    /*!
     * @brief Serialize a custom object array
     * @tparam T Object type
     * @param name JSON Object name
     * @param object Object array
     * @param serializer Custom serializer implementation
     * @return The current JSON Instance for method chaining.
     */
    template<class T> JSONBuilder& Field(const char* name, const Array<T*> objects, ISerializeToJson<const T>& serializer)
    {
      if (name != nullptr)
      {
        bool notEmpty = false;
        OpenArray(name);
        for(const T* object : objects)
          if (object != nullptr)
            if (JSONBuilder jsonObject = serializer.Serialize(object); !jsonObject.empty())
            {
              if (notEmpty) Append(',');
              Append(jsonObject);
              notEmpty = true;
            }
        CloseArray();
      }
      return *this;
    }

    /*!
     * @brief Serialize a custom object array
     * @tparam T Object type
     * @param name JSON Object name
     * @param object Object array
     * @param serializer Custom serializer implementation
     * @return The current JSON Instance for method chaining.
     */
    template<class T> JSONBuilder& Field(const char* name, const std::vector<T*> objects, ISerializeToJson<const T>& serializer)
    {
      if (name != nullptr)
      {
        bool notEmpty = false;
        OpenArray(name);
        for(const T* object : objects)
          if (object != nullptr)
            if (JSONBuilder jsonObject = serializer.Serialize(object); !jsonObject.empty())
            {
              if (notEmpty) Append(',');
              Append(jsonObject);
              notEmpty = true;
            }
        CloseArray();
      }
      return *this;
    }

    /*!
     * @brief Append raw data
     * @param raw raw data
     * @return This
     */
    JSONBuilder& AppendRaw(const String& raw) { Append(raw); return *this; }
};
