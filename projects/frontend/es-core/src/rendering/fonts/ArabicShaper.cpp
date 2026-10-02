//
// Created by bkg2k
//
#include "ArabicShaper.h"

// ============================================================================
// Join-type table for U+0600–U+06FF  (index = cp - 0x0600)
// U=0 Non-joining  R=1 Right-joining  D=2 Dual-joining
// C=3 Join-causing (Kashida)          T=4 Transparent (diacritics)
// ============================================================================
const ArabicShaper::JoinType ArabicShaper::sJT[256] =
{
  //  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
      U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U, // 060x
      T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  U,  U,  U,  U,  U, // 061x (0610-061A=T harakat, 061B-061F=U)
      D,  U,  R,  R,  R,  R,  D,  R,  D,  R,  D,  D,  D,  D,  D,  R, // 062x (0620=D,0621=U HAMZA,0622-0625=R,0626=D,0627=R,0628=D,0629=R,062A-062E=D,062F=R)
      R,  R,  R,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D, // 063x (0630-0632=R,0633-063F=D)
      C,  D,  D,  D,  D,  D,  D,  D,  R,  D,  D,  T,  T,  T,  T,  T, // 064x (0640=C Tatweel,0641-0647=D,0648=R WAW,0649-064A=D,064B-064F=T)
      T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T,  T, // 065x (0650-065F=T harakat)
      U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  D,  D, // 066x (0660-066D=U digits/punct,066E-066F=D)
      T,  R,  R,  R,  U,  R,  R,  R,  D,  D,  D,  D,  D,  D,  D,  D, // 067x (0670=T,0671-0673=R,0674=U,0675-0677=R,0678-067F=D)
      D,  D,  D,  D,  D,  D,  D,  D,  R,  R,  R,  R,  R,  R,  R,  R, // 068x (0680-0687=D,0688-068F=R)
      R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  D,  D,  D,  D,  D,  D, // 069x (0690-0699=R,069A-069F=D)
      D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  D, // 06Ax (06A0-06AF=D)
      D,  D,  D,  D,  D,  D,  D,  D,  D,  D,  R,  D,  D,  D,  D,  D, // 06Bx (06B0-06B9=D,06BA=R Noon Ghunna,06BB-06BF=D)
      R,  D,  R,  R,  R,  R,  R,  R,  R,  R,  R,  R,  D,  R,  D,  R, // 06Cx (06C0=R,06C1=D HehGoal,06C2-06CB=R,06CC=D FarsiYeh,06CD=R,06CE=D,06CF=R)
      D,  D,  R,  R,  U,  R,  T,  T,  T,  T,  T,  T,  T,  U,  U,  T, // 06Dx (06D0-06D1=D,06D2-06D3=R,06D4=U,06D5=R,06D6-06DC=T,06DD-06DE=U,06DF=T)
      T,  T,  T,  T,  T,  U,  U,  T,  T,  U,  T,  T,  T,  T,  R,  R, // 06Ex (06E0-06E4=T,06E5-06E6=U,06E7-06E8=T,06E9=U,06EA-06ED=T,06EE-06EF=R)
      U,  U,  U,  U,  U,  U,  U,  U,  U,  U,  D,  D,  D,  U,  U,  D, // 06Fx (06F0-06F9=U digits,06FA-06FC=D,06FD-06FE=U,06FF=D)
};

// ============================================================================
// Presentation-forms table for U+0600–U+06FF  (index = cp - 0x0600)
// {isolated, final, initial, medial};  0 = no distinct form.
// Sources: Unicode Presentation Forms-B (FE70-FEFF) and Forms-A (FB50-FDFF).
// ============================================================================
const ArabicShaper::Forms ArabicShaper::sForms[256] =
{
  // ---- 0600-061F : format chars, harakat ─ no letter forms ----------------
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0600-0607
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0608-060F
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0610-0617
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0618-061F

  // ---- 0620-063A : core Arabic consonants ---------------------------------
  {0,0,0,0},                          // 0620 (no standard presentation forms)
  {0xFE80,0,0,0},                     // 0621 HAMZA (isolated only)
  {0xFE81,0xFE82,0,0},               // 0622 ALEF WITH MADDA ABOVE
  {0xFE83,0xFE84,0,0},               // 0623 ALEF WITH HAMZA ABOVE
  {0xFE85,0xFE86,0,0},               // 0624 WAW WITH HAMZA ABOVE
  {0xFE87,0xFE88,0,0},               // 0625 ALEF WITH HAMZA BELOW
  {0xFE89,0xFE8A,0xFE8B,0xFE8C},    // 0626 YEH WITH HAMZA ABOVE
  {0xFE8D,0xFE8E,0,0},               // 0627 ALEF
  {0xFE8F,0xFE90,0xFE91,0xFE92},    // 0628 BA
  {0xFE93,0xFE94,0,0},               // 0629 TA MARBUTA
  {0xFE95,0xFE96,0xFE97,0xFE98},    // 062A TA
  {0xFE99,0xFE9A,0xFE9B,0xFE9C},    // 062B THA
  {0xFE9D,0xFE9E,0xFE9F,0xFEA0},    // 062C JEEM
  {0xFEA1,0xFEA2,0xFEA3,0xFEA4},    // 062D HAH
  {0xFEA5,0xFEA6,0xFEA7,0xFEA8},    // 062E KHA
  {0xFEA9,0xFEAA,0,0},               // 062F DAL
  {0xFEAB,0xFEAC,0,0},               // 0630 THAL
  {0xFEAD,0xFEAE,0,0},               // 0631 RA
  {0xFEAF,0xFEB0,0,0},               // 0632 ZAYN
  {0xFEB1,0xFEB2,0xFEB3,0xFEB4},    // 0633 SEEN
  {0xFEB5,0xFEB6,0xFEB7,0xFEB8},    // 0634 SHEEN
  {0xFEB9,0xFEBA,0xFEBB,0xFEBC},    // 0635 SAD
  {0xFEBD,0xFEBE,0xFEBF,0xFEC0},    // 0636 DAD
  {0xFEC1,0xFEC2,0xFEC3,0xFEC4},    // 0637 TAH
  {0xFEC5,0xFEC6,0xFEC7,0xFEC8},    // 0638 ZAH
  {0xFEC9,0xFECA,0xFECB,0xFECC},    // 0639 AIN
  {0xFECD,0xFECE,0xFECF,0xFED0},    // 063A GHAIN

  // ---- 063B-063F : extended letters (no Unicode presentation forms) -------
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 063B-063F

  // ---- 0640-064A : Tatweel + remaining consonants -------------------------
  {0,0,0,0},                          // 0640 TATWEEL (kashida, no form change)
  {0xFED1,0xFED2,0xFED3,0xFED4},    // 0641 FA
  {0xFED5,0xFED6,0xFED7,0xFED8},    // 0642 QAF
  {0xFED9,0xFEDA,0xFEDB,0xFEDC},    // 0643 KAF
  {0xFEDD,0xFEDE,0xFEDF,0xFEE0},    // 0644 LAM
  {0xFEE1,0xFEE2,0xFEE3,0xFEE4},    // 0645 MEEM
  {0xFEE5,0xFEE6,0xFEE7,0xFEE8},    // 0646 NOON
  {0xFEE9,0xFEEA,0xFEEB,0xFEEC},    // 0647 HEH
  {0xFEED,0xFEEE,0,0},               // 0648 WAW
  {0xFEEF,0xFEF0,0,0},               // 0649 ALEF MAQSURA
  {0xFEF1,0xFEF2,0xFEF3,0xFEF4},    // 064A YEH

  // ---- 064B-066D : harakat + Arabic-Indic digits + punctuation -----------
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 064B-064F
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0650-0654
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0655-0659
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 065A-065E
  {0,0,0,0},                                          // 065F
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0660-0664
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0665-0669
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},            // 066A-066D

  // ---- 066E-066F : Dotless BA / QAF (no standard presentation forms) -----
  {0,0,0,0},{0,0,0,0}, // 066E-066F

  // ---- 0670-067F : Arabic Supplement ─ select letters with FB50-FDFF forms
  {0,0,0,0},                          // 0670 SUPERSCRIPT ALEF (transparent)
  {0xFB50,0xFB51,0,0},               // 0671 ALEF WASLA
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0672-0678
  {0xFB66,0xFB67,0xFB68,0xFB69},    // 0679 TA WITH SMALL TAH (Tte)
  {0xFB5E,0xFB5F,0xFB60,0xFB61},    // 067A TA WITH TWO LOOPS
  {0xFB52,0xFB53,0xFB54,0xFB55},    // 067B BEH WITH THREE DOTS UPWARDS BELOW
  {0,0,0,0},{0,0,0,0},               // 067C-067D
  {0xFB56,0xFB57,0xFB58,0xFB59},    // 067E PEH
  {0xFB6A,0xFB6B,0xFB6C,0xFB6D},    // 067F TA WITH FOUR DOTS ABOVE

  // ---- 0680-068F ----------------------------------------------------------
  {0xFB5A,0xFB5B,0xFB5C,0xFB5D},    // 0680 BEH WITH DOT BELOW AND THREE DOTS ABOVE
  {0,0,0,0},{0,0,0,0},               // 0681-0682
  {0xFB76,0xFB77,0xFB78,0xFB79},    // 0683 NYA (HAH WITH TWO DOTS BELOW)
  {0xFB72,0xFB73,0xFB74,0xFB75},    // 0684 HAH WITH TWO DOTS BELOW AND ABOVE
  {0,0,0,0},                          // 0685
  {0xFB7A,0xFB7B,0xFB7C,0xFB7D},    // 0686 TCHEH
  {0xFB7E,0xFB7F,0xFB80,0xFB81},    // 0687 TCHEH WITH FOUR DOTS ABOVE
  {0xFB88,0xFB89,0,0},               // 0688 DAL WITH RING
  {0,0,0,0},{0,0,0,0},{0,0,0,0},     // 0689-068B
  {0xFB84,0xFB85,0,0},               // 068C DAL WITH TWO DOTS BELOW
  {0xFB82,0xFB83,0,0},               // 068D DAL WITH TWO DOTS
  {0xFB86,0xFB87,0,0},               // 068E DAL WITH THREE DOTS ABOVE
  {0,0,0,0},                          // 068F

  // ---- 0690-069F ----------------------------------------------------------
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0690-0697
  {0xFB8A,0xFB8B,0,0},               // 0698 JEH (RA WITH THREE DOTS ABOVE)
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 0699-069F

  // ---- 06A0-06AF ----------------------------------------------------------
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06A0-06A3
  {0xFB6E,0xFB6F,0xFB70,0xFB71},    // 06A4 VEH
  {0,0,0,0},                          // 06A5
  {0xFB8C,0xFB8D,0,0},               // 06A6 PEHEH (only iso+fin known)
  {0,0,0,0},{0,0,0,0},               // 06A7-06A8
  {0xFB8E,0xFB8F,0xFB90,0xFB91},    // 06A9 KEHEH
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06AA-06AE
  {0xFB92,0xFB93,0xFB94,0xFB95},    // 06AF GAAF

  // ---- 06B0-06BF ----------------------------------------------------------
  {0,0,0,0},                          // 06B0
  {0xFB9A,0xFB9B,0xFB9C,0xFB9D},    // 06B1 GAAF WITH TWO DOTS ABOVE (Ngoeh)
  {0,0,0,0},                          // 06B2
  {0xFB96,0xFB97,0xFB98,0xFB99},    // 06B3 GAAF WITH THREE DOTS ABOVE (Gueh)
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06B4-06B9
  {0xFB9E,0xFB9F,0,0},               // 06BA NOON GHUNNA (right-joining)
  {0xFBA0,0xFBA1,0xFBA2,0xFBA3},    // 06BB RNOON
  {0,0,0,0},{0,0,0,0},               // 06BC-06BD
  {0xFBAA,0xFBAB,0xFBAC,0xFBAD},    // 06BE HEH DOACHASHMEE
  {0,0,0,0},                          // 06BF

  // ---- 06C0-06CF ----------------------------------------------------------
  {0xFBA4,0xFBA5,0,0},               // 06C0 HEH WITH YEH ABOVE
  {0xFBA6,0xFBA7,0xFBA8,0xFBA9},    // 06C1 HEH GOAL
  {0,0,0,0},{0,0,0,0},               // 06C2-06C3
  {0,0,0,0},                          // 06C4
  {0xFBE0,0xFBE1,0,0},               // 06C5 KIRGIZ OE
  {0xFBD9,0xFBDA,0,0},               // 06C6 OE
  {0xFBD7,0xFBD8,0,0},               // 06C7 U
  {0xFBDB,0xFBDC,0,0},               // 06C8 YU
  {0xFBE3,0,0,0},                    // 06C9 KIRGIZ YU
  {0,0,0,0},                          // 06CA
  {0xFBDE,0xFBDF,0,0},               // 06CB VE
  {0xFBFC,0xFBFD,0xFBFE,0xFBFF},    // 06CC FARSI YEH
  {0,0,0,0},                          // 06CD
  {0,0,0,0},                          // 06CE
  {0,0,0,0},                          // 06CF

  // ---- 06D0-06FF ----------------------------------------------------------
  {0xFBE4,0xFBE5,0xFBE6,0xFBE7},    // 06D0 E
  {0,0,0,0},                          // 06D1
  {0xFBAE,0xFBAF,0,0},               // 06D2 YEH BARREE
  {0xFBB0,0xFBB1,0,0},               // 06D3 YEH BARREE WITH HAMZA ABOVE
  {0,0,0,0},                          // 06D4 ARABIC FULL STOP
  {0xFBE8,0xFBE9,0,0},               // 06D5 AE
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06D6-06DC
  {0,0,0,0},{0,0,0,0},{0,0,0,0},     // 06DD-06DF
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06E0-06E4
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06E5-06E9
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06EA-06EE
  {0,0,0,0},                          // 06EF
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06F0-06F4
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06F5-06F9
  {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}, // 06FA-06FF
};

// ============================================================================
// HasArabic — fast byte-level scan, no full UTF-8 decode
// ============================================================================
bool ArabicShaper::HasArabic(const String& string, int start, int length) noexcept
{
  const char* p   = string.data() + start;
  const char* end = p + length;
  while (p < end)
  {
    unsigned char b = (unsigned char)*p;
    // Arabic 0x0600-0x06FF and Supplement 0x0750-0x077F:
    //   2-byte UTF-8 with first byte 0xD8-0xDD
    if (b >= 0xD8u && b <= 0xDDu) return true;
    // Arabic Presentation Forms (FB50-FDFF, FE70-FEFF):
    //   3-byte UTF-8 starting with 0xEF; second byte >= 0xAD
    if (b == 0xEFu && p + 1 < end && (unsigned char)*(p + 1) >= 0xADu) return true;
    p++;
  }
  return false;
}

bool ArabicShaper::StartsRTL(const String& text) noexcept
{
  const unsigned char* p = (const unsigned char*)text.data();
  const unsigned char* e = p + text.length();
  while (p < e)
  {
    unsigned char b = *p;
    if (b <= 0x20u) { p++; continue; }          // space / control: skip (neutral)
    if (b < 0x80u)                               // ASCII non-space
    {
      // Letters a-z A-Z are LTR strong; digits and punctuation are weak/neutral
      if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z')) return false;
      p++; continue;                             // digit / punctuation: keep scanning
    }
    // Arabic U+0600-U+077F: 2-byte UTF-8 first byte 0xD8-0xDD → RTL
    if (b >= 0xD8u && b <= 0xDDu) return true;
    return false;                                // any other multibyte script → LTR
  }
  return false;
}

// ============================================================================
// ShapeRun — shape one Arabic run in logical order, append reversed to `out`
// ============================================================================
void ArabicShaper::ShapeRun(const unsigned int* run, int len, Array<unsigned int>& out)
{
  if (len <= 0) return;

  // -- Step 1 : Lam-Alef mandatory ligature substitution --------------------
  // We collapse each (LAM, ALEF-type) pair into a single synthetic entry.
  // isLiga=true entries store the Alef codepoint; the Lam is consumed.
  //
  // Joining logic for a liga entry (position j in temp):
  //   right-face (toward lower indices) = LAM = D  → CanReceive  = YES
  //   left-face  (toward higher indices) = ALEF = R → CanSend     = NO

  struct Entry { unsigned int cp; bool isLiga; };
  static constexpr int kMaxRun = 256;
  Entry temp[kMaxRun];
  int   tempLen = 0;

  for (int i = 0; i < len && tempLen < kMaxRun; )
  {
    if (run[i] == 0x0644u && i + 1 < len && IsAlefForLiga(run[i + 1]))
    {
      temp[tempLen++] = { run[i + 1], true }; // store Alef cp, mark as liga
      i += 2;
    }
    else
    {
      temp[tempLen++] = { run[i++], false };
    }
  }

  // -- Step 2 : Determine contextual form for each entry --------------------
  unsigned int shaped[kMaxRun];

  for (int i = 0; i < tempLen; i++)
  {
    const Entry& e = temp[i];

    // Find nearest non-transparent entry BEFORE i (scanning right in visual terms).
    // We want its effective "CanSend toward left" ability:
    //   - normal char: its own join type
    //   - liga entry: ALEF face toward higher indices → R (cannot send)
    JoinType prevJT = U;
    for (int j = i - 1; j >= 0; j--)
    {
      JoinType jt = temp[j].isLiga ? R : GetJT(temp[j].cp);
      if (jt != T) { prevJT = jt; break; }
    }

    // Find nearest non-transparent entry AFTER i (scanning left in visual terms).
    // We want its effective "CanReceive from right" ability:
    //   - normal char: its own join type
    //   - liga entry: LAM face toward lower indices → D (can receive)
    JoinType nextJT = U;
    for (int j = i + 1; j < tempLen; j++)
    {
      JoinType jt = temp[j].isLiga ? D : GetJT(temp[j].cp);
      if (jt != T) { nextJT = jt; break; }
    }

    if (e.isLiga)
    {
      // LAM can receive (D). Connect iff prevJT can send.
      bool connectsRight = (prevJT == D || prevJT == C);
      shaped[i] = LamAlef(e.cp, connectsRight);
    }
    else
    {
      JoinType myJT = GetJT(e.cp);
      bool connectsRight = (prevJT == D || prevJT == C) && (myJT == D || myJT == R);
      bool connectsLeft  = (myJT  == D || myJT  == C) && (nextJT == D || nextJT == R);
      shaped[i] = GetForm(e.cp, connectsRight, connectsLeft);
    }
  }

  // -- Step 3 : Append in REVERSED order (Arabic runs render right-to-left) -
  for (int i = tempLen; --i >= 0; )
    out.Add(shaped[i]);
}

// ============================================================================
// Process — main entry point
// ============================================================================
void ArabicShaper::Process(const String& string, int start, int length, Array<unsigned int>& out)
{
  out.Clear();

  // Stack buffer for the current Arabic run (words are short; 256 is generous).
  static constexpr int kRunBuf = 256;
  unsigned int runBuf[kRunBuf];
  int  runLen      = 0;
  bool inArabicRun = false;

  auto flushRun = [&]()
  {
    if (runLen > 0) ShapeRun(runBuf, runLen, out);
    runLen      = 0;
    inArabicRun = false;
  };

  int end = start + length;
  int pos = start;
  while (pos < end)
  {
    unsigned int cp     = string.ReadUTF8(pos);
    bool         isArab = IsArabic(cp);

    if (isArab)
    {
      if (!inArabicRun) inArabicRun = true;
      if (runLen < kRunBuf)
        runBuf[runLen++] = cp;
      else
      {
        // Overflow: flush current run, start fresh (very rare for any real text)
        flushRun();
        inArabicRun      = true;
        runBuf[runLen++] = cp;
      }
    }
    else
    {
      if (inArabicRun) flushRun();
      out.Add(cp);
    }
  }

  if (inArabicRun) flushRun();
}
