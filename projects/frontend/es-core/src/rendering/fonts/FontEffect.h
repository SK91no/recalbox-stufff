//
// Created by bkg2k on 13/12/2020.
//
#pragma once

struct FontEffect
{
  private:
    //! Give font class direct access
    friend class NewFont;

    //! Final structure
    struct Final
    {
      float HorizontalScaling; //!< Horizontal scaling ratio
      float VerticalScaling;   //!< Vertical scaling ratio (in respect of the vertical origin, baseline or any other)
      float Inclination;       //!< Top fond inclination ratio (ratio applied to the font max height to get pixel offset)
      bool Bold;               //!< If set, the font is drawn twice at X and X+1
    };

    //! Original structure
    struct
    {
      float HorizontalScaling; //!< Horizontal scaling ratio
      float VerticalScaling;   //!< Vertical scaling ratio (in respect of the vertical origin, baseline or any other)
      float Inclination;       //!< Top fond inclination ratio (ratio applied to the font max height to get pixel offset)
      bool Italic;             //!< If set, provide an additional inclination of 25%
      bool Bold;               //!< If set, the font is drawn twice at X and X+1
    }
    Original; //! Original values as set by the caller

    //! Finalize computations
    [[nodiscard]] Final Finalize() const
    {
      Final f { .HorizontalScaling=Original.HorizontalScaling, .VerticalScaling=Original.VerticalScaling, .Inclination=Original.Inclination, .Bold=Original.Bold };
      if (Original.Italic) f.Inclination += 0.25f;
      return f;
    }

  public:
    //! Default constructor - no effect
    FontEffect()
      : Original{ .HorizontalScaling=1.0f, .VerticalScaling=1.0f, .Inclination=0.0f, .Italic=false, .Bold=false }
    {
    }

    void Reset()
    {
      Original = { .HorizontalScaling=1.0f, .VerticalScaling=1.0f, .Inclination=0.0f, .Italic=false, .Bold=false };
    }

    //! Set Italic
    FontEffect& SetItalic(bool on)
    {
      Original.Italic = on;
      return *this;
    }

    //! Set Bold
    FontEffect& SetBold(bool on)
    {
      Original.Bold = on;
      return *this;
    }

    //! Set inclination
    FontEffect& SetInclination(float inclination)
    {
      Original.Inclination = inclination;
      return *this;
    }

    //! Set scaling
    FontEffect& SetScaling(float horizontal, float vertical)
    {
      Original.HorizontalScaling = horizontal;
      Original.VerticalScaling = vertical;
      return *this;
    }

    //! Set scaling
    FontEffect& SetHScaling(float horizontal)
    {
      Original.HorizontalScaling = horizontal;
      return *this;
    }

    //! Set scaling
    FontEffect& SetVScaling(float vertical)
    {
      Original.VerticalScaling = vertical;
      return *this;
    }

    //! Deserialize from the given string
    FontEffect& Deserialize(const String& string)
    {
      Original.Italic = Original.Bold = false;
      for(char c : string)
        switch(c | 32)
        {
          case 'i': Original.Italic = true; break;
          case 'b': Original.Bold = true; break;
          default: break;
        }
      return *this;
    }
};
