//
// Created by thierry.imbert on 10/01/2020.
//
#pragma once

#include "rendering/RenderingError.h"

class IUnloadable
{
  public:
    //! Load the resource
    virtual RenderingError Load() = 0;

    //! Unload the resource
    virtual void Unload() = 0;

    //! Free CPU caches if any
    virtual void UnloadCaches() = 0;
};
