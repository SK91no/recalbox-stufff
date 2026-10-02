//
// Arabic text shaper: contextual form selection + BiDi run reversal.
// Performance design: all lookup tables are O(1) static arrays, no heap
// allocation in the hot path. Stack buffers are used for Arabic run processing.
//
#pragma once

#include <utils/storage/Array.h>
#include <utils/String.h>

class ArabicShaper
{
  public:
    // Returns true if [start, start+length) in string contains any Arabic character.
    // Fast byte-level scan — no full UTF-8 decode required.
    static bool HasArabic(const String& string, int start, int length) noexcept;

    // Returns true if the paragraph base direction of `text` is RTL.
    // Scans for the first strong directional character (UAX#9 P2/P3 heuristic):
    // skips ASCII whitespace/punctuation/digits, returns true on first Arabic byte sequence.
    static bool StartsRTL(const String& text) noexcept;

    // Convert a UTF-8 string segment into a shaped + BiDi-reordered codepoint
    // array ready for left-to-right rendering:
    //   - Arabic runs are contextually shaped and reversed.
    //   - Non-Arabic runs are copied as-is.
    static void Process(const String& string, int start, int length, Array<unsigned int>& out);

    // True if cp can send a connection toward following chars (join type D or C).
    static bool JoinsSent(unsigned int cp) noexcept     { JoinType jt = GetJT(cp); return jt == D || jt == C; }
    // True if cp can receive a connection from preceding chars (join type D or R).
    static bool JoinsReceived(unsigned int cp) noexcept { JoinType jt = GetJT(cp); return jt == D || jt == R; }

    // True when cp is an Alef variant that forms a mandatory Lam-Alef ligature.
    static bool IsAlefForLiga(unsigned int cp) noexcept
    {
      return cp == 0x0622u || cp == 0x0623u || cp == 0x0625u || cp == 0x0627u || cp == 0x0671u;
    }

    // Lam-Alef ligature codepoint. connectsRight=true → final form (Lam connects right).
    static unsigned int LamAlef(unsigned int alef, bool connectsRight) noexcept
    {
      switch (alef)
      {
        case 0x0622u: return connectsRight ? 0xFEF6u : 0xFEF5u;
        case 0x0623u: return connectsRight ? 0xFEF8u : 0xFEF7u;
        case 0x0625u: return connectsRight ? 0xFEFAu : 0xFEF9u;
        case 0x0671u:
        case 0x0627u: return connectsRight ? 0xFEFCu : 0xFEFBu;
        default:      return connectsRight ? 0xFEFCu : 0xFEFBu;
      }
    }

    // Presentation-form codepoint for cp in context (for glyph measurement).
    static unsigned int ContextualForm(unsigned int cp, bool connectsRight, bool connectsLeft) noexcept
    {
      return GetForm(cp, connectsRight, connectsLeft);
    }

  private:
    // -----------------------------------------------------------------------
    // Join types (Unicode Bidirectional Character Types, joining subset)
    // -----------------------------------------------------------------------
    enum JoinType : unsigned char
    {
      U = 0, // Non-joining
      R = 1, // Right-joining  (Alef, Dal, Ra …)
      D = 2, // Dual-joining   (Ba, Ta, Lam …)
      C = 3, // Join-causing   (Tatweel / Kashida)
      T = 4, // Transparent    (harakat / diacritics – skipped for joining)
    };

    // Four contextual presentation-form codepoints for one Arabic base letter.
    // A value of 0 means "no distinct form – fall back to isolated".
    struct Forms
    {
      unsigned short iso; // isolated
      unsigned short fin; // final
      unsigned short ini; // initial
      unsigned short med; // medial
    };

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // True when the codepoint belongs to an Arabic script block.
    static bool IsArabic(unsigned int cp) noexcept
    {
      return (cp >= 0x0600u && cp <= 0x06FFu)   // Arabic + Arabic Supplement
          || (cp >= 0x0750u && cp <= 0x077Fu)   // Arabic Supplement
          || (cp >= 0xFB50u && cp <= 0xFDFFu)   // Arabic Pres. Forms-A
          || (cp >= 0xFE70u && cp <= 0xFEFFu);  // Arabic Pres. Forms-B
    }

    // Joining type for a codepoint (used for contextual form selection).
    static JoinType GetJT(unsigned int cp) noexcept
    {
      if (cp >= 0x0600u && cp <= 0x06FFu) return sJT[cp - 0x0600u];
      // Presentation forms: already shaped, treat as non-joining for safety
      return U;
    }

    // Return the contextual Unicode presentation form for an Arabic codepoint.
    static unsigned int GetForm(unsigned int cp, bool connectsRight, bool connectsLeft) noexcept
    {
      // Already a presentation form – use as-is (no double-shaping)
      if ((cp >= 0xFB50u && cp <= 0xFDFFu) || (cp >= 0xFE70u && cp <= 0xFEFFu)) return cp;
      if (cp < 0x0600u || cp > 0x06FFu) return cp;

      const Forms& f = sForms[cp - 0x0600u];
      if (f.iso == 0u) return cp; // no presentation forms available

      if (connectsRight && connectsLeft) return f.med ? f.med : (f.fin ? f.fin : f.iso);
      if (connectsRight)                 return f.fin ? f.fin : f.iso;
      if (connectsLeft)                  return f.ini ? f.ini : f.iso;
      return f.iso;
    }

    // Shape one Arabic run (logical order) and append reversed + shaped codepoints to `out`.
    static void ShapeRun(const unsigned int* run, int len, Array<unsigned int>& out);

    // -----------------------------------------------------------------------
    // Static tables (defined in ArabicShaper.cpp)
    // -----------------------------------------------------------------------
    static const JoinType sJT[256];    // indexed by (cp - 0x0600)
    static const Forms    sForms[256]; // indexed by (cp - 0x0600)
};
