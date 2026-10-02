//
// Created by bkg2k on 01/12/23.
//
#pragma once

// Forward declaration
class JSONBuilder;

template<class T> class ISerializeToJson
{
  public:
    //! Virtual destructor
    virtual ~ISerializeToJson() = default;

    /*!
     * @brief Serialize the given object and return an appropriate JSON object
     * @param object Object to serialize
     * @return Object serialized to JSON
     */
    virtual JSONBuilder Serialize(const T* object) = 0;
};
