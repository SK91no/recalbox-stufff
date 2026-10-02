//
// Created by bkg2k on 28/09/2020.
//
// COLOR CONVENTION — read this before touching anything here.
//
// Every color in the frontend is a packed 32bit value laid out as:
//
//     0xRRGGBBAA        (alpha in the LOW byte)
//
// This is *not* the #AARRGGBB notation used by external tools such as
// Photoshop. Use FromARGB()/ToARGB() at the boundary when talking to them.
//
// The convention is imposed by the two ends of the pipeline:
//  - theme parsing (ThemeData::GetColor) turns "#RRGGBB" into 0xRRGGBBFF,
//  - components everywhere read the alpha as `color & 0xFF`,
//  - Renderer::ColorToGL byte-swaps it into the R,G,B,A byte order the GPU
//    expects for a GL_UNSIGNED_BYTE / VK_FORMAT_R8G8B8A8_UNORM attribute.
//
// Historical note: this file used to be written for the #AARRGGBB layout and
// was silently wrong for years — every one of those helpers had zero callers,
// which is why nothing broke. The only visible casualty was Common::Blue,
// which rendered red. Do not reintroduce ARGB arithmetic here.
//
#pragma once

#include "rendering/RenderingTypes.h"

class Colors
{
  public:
    //! Convenient color array for Triangle
    typedef ColorByte TriangleColors[3 * 4];
    //! Convenient color array for Rectangle
    typedef ColorByte RectangleColors[6 * 4];

    //! Packed color, 0xRRGGBBAA — the canonical spelling
    typedef unsigned int ColorRGBA;
    //! Legacy spelling of ColorRGBA. The name says ARGB, the layout is and has
    //! always been RGBA. Kept because hundreds of call sites still use it;
    //! prefer ColorRGBA in new code.
    typedef unsigned int ColorARGB;
    //! Single color component
    typedef unsigned char Component;

    static constexpr ColorRGBA None = 0x00000000;

    class Common
    {
      public:
        static constexpr ColorRGBA Transparent = 0x00000000;
        static constexpr ColorRGBA Black       = 0x000000FF;
        static constexpr ColorRGBA White       = 0xFFFFFFFF;
        static constexpr ColorRGBA Red         = 0xFF0000FF;
        static constexpr ColorRGBA Green       = 0x00FF00FF;
        static constexpr ColorRGBA Blue        = 0x0000FFFF;
        static constexpr ColorRGBA Purple      = 0xFF00FFFF;
        static constexpr ColorRGBA Yellow      = 0xFFFF00FF;
        static constexpr ColorRGBA Cyan        = 0x00FFFFFF;
    };

    /*!
     * @brief Build a fully opaque color from its three components
     * @param red Red component
     * @param green Green component
     * @param blue Blue component
     * @return Packed RGBA color
     */
    static ColorRGBA MakeRGB(Component red, Component green, Component blue)
    { return ((ColorRGBA)red << 24) | ((ColorRGBA)green << 16) | ((ColorRGBA)blue << 8) | 0xFF; }

    /*!
     * @brief Build a color from its four components
     * @param red Red component
     * @param green Green component
     * @param blue Blue component
     * @param alpha Alpha component
     * @return Packed RGBA color
     */
    static ColorRGBA MakeRGBA(Component red, Component green, Component blue, Component alpha)
    { return ((ColorRGBA)red << 24) | ((ColorRGBA)green << 16) | ((ColorRGBA)blue << 8) | (ColorRGBA)alpha; }

    /*!
     * @brief Convert from the #AARRGGBB notation used by external tools
     * @param argb Color in 0xAARRGGBB form
     * @return Packed RGBA color
     */
    static ColorRGBA FromARGB(unsigned int argb) { return (argb << 8) | (argb >> 24); }

    /*!
     * @brief Convert to the #AARRGGBB notation used by external tools
     * @param rgba Packed RGBA color
     * @return Color in 0xAARRGGBB form
     */
    static unsigned int ToARGB(ColorRGBA rgba) { return (rgba >> 8) | (rgba << 24); }

    /*!
     * @brief Get Alpha component
     * @param color Input Color
     * @return Alpha component
     */
    static Component Alpha(ColorRGBA color) { return color & 0xFF; }
    /*!
     * @brief Get Red component
     * @param color Input Color
     * @return Red component
     */
    static Component Red(ColorRGBA color) { return color >> 24; }
    /*!
     * @brief Get Green component
     * @param color Input Color
     * @return Green component
     */
    static Component Green(ColorRGBA color) { return (color >> 16) & 0xFF; }
    /*!
     * @brief Get Blue component
     * @param color Input Color
     * @return Blue component
     */
    static Component Blue(ColorRGBA color) { return (color >> 8) & 0xFF; }
    /*!
     * @brief Get pure color RGB, w/o Alpha (alpha bits zeroed)
     * @param color Input color
     * @return Pure color
     */
    static ColorRGBA PureColor(ColorRGBA color) { return color & 0xFFFFFF00; }

    /*!
     * @brief Replace the given color's alpha component with the given one
     * @param color Source color
     * @param alpha Alpha component
     * @return Source color with the Alpha component replaced
     */
    static ColorRGBA SetAlpha(ColorRGBA color, Component alpha) { return (color & 0xFFFFFF00) | (ColorRGBA)alpha; }

    /*!
     * @brief Replace the given color's red component with the given one
     * @param color Source color
     * @param red Red component
     * @return Source color with the Red component replaced
     */
    static ColorRGBA SetRed(ColorRGBA color, Component red) { return (color & 0x00FFFFFF) | ((ColorRGBA)red << 24); }

    /*!
     * @brief Replace the given color's green component with the given one
     * @param color Source color
     * @param green Green component
     * @return Source color with the Green component replaced
     */
    static ColorRGBA SetGreen(ColorRGBA color, Component green) { return (color & 0xFF00FFFF) | ((ColorRGBA)green << 16); }

    /*!
     * @brief Replace the given color's blue component with the given one
     * @param color Source color
     * @param blue Blue component
     * @return Source color with the Blue component replaced
     */
    static ColorRGBA SetBlue(ColorRGBA color, Component blue) { return (color & 0xFFFF00FF) | ((ColorRGBA)blue << 8); }

    /*!
     * @brief Check if the alpha component of the given color is not zero
     * @param color Color to check
     * @return True if the alpha component is > 0
     */
    static bool IsVisible(ColorRGBA color) { return (color & 0xFF) != 0; }

    /*!
     * @brief Convert a packed color into the R,G,B,A byte order the GPU expects
     * @param rgba Packed RGBA color
     * @return Value whose in-memory byte order is R,G,B,A
     */
    static ColorRGBA ToGPUBytes(ColorRGBA rgba)
    {
      #ifdef LITTLE_ENDIAN
      return __builtin_bswap32(rgba);
      #else
      return rgba;
      #endif
    }

    /*!
     * @brief Store one color, GPU byte ordered, into a color byte array
     * @param destination Destination byte array (4 bytes written)
     * @param rgba Source packed RGBA color
     */
    static void StoreGPUColor(ColorByte* destination, ColorRGBA rgba)
    {
      *((ColorRGBA*)destination) = ToGPUBytes(rgba);
    }

    /*!
     * @brief Fill a rectangle color array with a single GPU byte ordered color
     * @param destination Destination array
     * @param rgba Source packed RGBA color
     */
    static void FillGPUColors(RectangleColors& destination, ColorRGBA rgba)
    {
      ColorRGBA* target = (ColorRGBA*)destination;
      ColorRGBA value = ToGPUBytes(rgba);
      for(int i = 6; --i >= 0; )
        target[i] = value;
    }

    /*!
     * @brief Fill a triangle color array with a single GPU byte ordered color
     * @param destination Destination array
     * @param rgba Source packed RGBA color
     */
    static void FillGPUColors(TriangleColors& destination, ColorRGBA rgba)
    {
      ColorRGBA* target = (ColorRGBA*)destination;
      ColorRGBA value = ToGPUBytes(rgba);
      for(int i = 3; --i >= 0; )
        target[i] = value;
    }

    /*!
     * @brief Repeat a GPU byte ordered color X times into the destination array
     * @param destination Destination array
     * @param rgba Source packed RGBA color
     * @param count Number of time to repeat the color in the destination array
     */
    static void FillGPUColorArray(ColorByte* destination, ColorRGBA rgba, int count)
    {
      ColorRGBA* target = (ColorRGBA*)destination;
      ColorRGBA value = ToGPUBytes(rgba);
      for(int i = count; --i >= 0; )
        target[i] = value;
    }

    /*!
     * @brief Apply a given color onto another, blending the applied color regarding its alpha component
     * @param appliedColor Color to apply
     * @param onto Original color
     * @return Blended color, keeping the destination alpha
     */
    static ColorRGBA ApplyColorOver(ColorRGBA appliedColor, ColorRGBA onto)
    {
      unsigned int as = appliedColor & 0xFF;
      unsigned int ad = 255 - as;
      unsigned int r = ((((appliedColor >> 24) & 0xFF) * as) + (((onto >> 24) & 0xFF) * ad)) >> 8;
      unsigned int g = ((((appliedColor >> 16) & 0xFF) * as) + (((onto >> 16) & 0xFF) * ad)) >> 8;
      unsigned int b = ((((appliedColor >>  8) & 0xFF) * as) + (((onto >>  8) & 0xFF) * ad)) >> 8;
      return (r << 24) | (g << 16) | (b << 8) | (onto & 0xFF);
    }

    /*!
     * @brief Apply the alpha component to RGB components and set alpha to full opaque
     * @param source Source color
     * @return Opaque color
     */
    static ColorRGBA AlphaToOpaque(ColorRGBA source)
    {
      unsigned int a = source & 0xFF;
      unsigned int r = (((source >> 24) & 0xFF) * a) >> 8;
      unsigned int g = (((source >> 16) & 0xFF) * a) >> 8;
      unsigned int b = (((source >>  8) & 0xFF) * a) >> 8;
      return (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }

    /*!
     * @brief Blend a color using the given ratio from 0 to 1
     * @param from Source color
     * @param to Destination color
     * @param ratio Destination ratio (0 = full source, 1 = full destination)
     * @return Blended color
     */
    static ColorRGBA Blend(ColorRGBA from, ColorRGBA to, float ratio)
    {
      int r = (int)(ratio * 255);
      int rs = (int)(from >> 24);
      int gs = (int)((from >> 16) & 0xFF);
      int bs = (int)((from >>  8) & 0xFF);
      int as = (int)((from >>  0) & 0xFF);
      int rd = (int)(to >> 24);
      int gd = (int)((to >> 16) & 0xFF);
      int bd = (int)((to >>  8) & 0xFF);
      int ad = (int)((to >>  0) & 0xFF);
      int rr = rs + (((rd - rs) * r) >> 8);
      int gr = gs + (((gd - gs) * r) >> 8);
      int br = bs + (((bd - bs) * r) >> 8);
      int ar = as + (((ad - as) * r) >> 8);
      return ((ColorRGBA)rr << 24) | ((ColorRGBA)gr << 16) | ((ColorRGBA)br << 8) | (ColorRGBA)ar;
    }

    /*!
     * @brief Darken a color using the given ratio from 0 to 1
     * @param originalColor Source color
     * @param darkness Destination ratio (0 = no change, 1 = black)
     * @return Darkened color
     */
    static ColorRGBA ApplyDarkness(ColorRGBA originalColor, float darkness)
    {
      //just in case darkness > 1 or < 0
      if (darkness > 1.0f)
        darkness = 1.0f;
      if (darkness < 0.0f)
        darkness = 0.0f;
      Component R = ((originalColor >> 24) & 0xFF) * (1 - darkness);
      Component G = ((originalColor >> 16) & 0xFF) * (1 - darkness);
      Component B = ((originalColor >>  8) & 0xFF) * (1 - darkness);
      return ((ColorRGBA)R << 24) | ((ColorRGBA)G << 16) | ((ColorRGBA)B << 8) | 0xFF;
    }

};
