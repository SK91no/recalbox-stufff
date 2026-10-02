//
// Created by bkg2k on 03/09/25.
//
#pragma once

//! Texture unloading strategy
enum class TextureUnload
{
  GpuOnly, //!< Free the GPU texture only, keep the CPU cache (IImage) for fast re-upload
  Full,    //!< Free both the GPU texture and the CPU cache
};

class ITextureCacheControl
{
  public:
    //! Default destructor
    virtual ~ITextureCacheControl() = default;

    /*!
     * @brief Unload the oldest GPU-resident texture to free memory
     * @param mode GpuOnly = keep the CPU cache, Full = drop the CPU cache too
     * @return Amount of memory freed in bytes (0 if no more unloadable texture)
     */
    [[nodiscard]] virtual long long UnloadTheOldest(TextureUnload mode) = 0;
};