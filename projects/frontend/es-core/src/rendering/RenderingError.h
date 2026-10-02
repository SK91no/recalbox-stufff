//
// Created by bkg2k on 19/04/24.
//
#pragma once

//! Error status
enum class RenderingError
{
  NoError,        //!< Everything is fine
  OutOfMemory,    //!< Out of memory (CPU)
  OutOfGPUMemory, //!< Out of memory (GPU)
  NoResource,     //!< File not found or unknown internal resource
};
