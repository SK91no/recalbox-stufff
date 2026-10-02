//
// Hardcoded data for the 6 keyboard layouts supported by Recalbox
// (us, fr, de, dk, es, it). Matches MenuDataProvider::GetKeyboardEntries().
//
#include "VKLayout.h"

// ---------- Convenience macros ----------
// Char keys: normal-only, normal+shifted, or full triple (normal/shift/altgr).
#define C1(n)          { VKKeyKind::Char, (unsigned int)(n), (unsigned int)(n), 0u, 100, nullptr }
#define C2(n, s)       { VKKeyKind::Char, (unsigned int)(n), (unsigned int)(s), 0u, 100, nullptr }
#define C3(n, s, a)    { VKKeyKind::Char, (unsigned int)(n), (unsigned int)(s), (unsigned int)(a), 100, nullptr }

// Special keys: widthx100 + label.
#define BS             { VKKeyKind::Backspace, 0u, 0u, 0u, 150, "Bksp"  }
#define ENT            { VKKeyKind::Enter,     0u, 0u, 0u, 150, "Enter" }
#define SHF            { VKKeyKind::Shift,     0u, 0u, 0u, 150, "Shift" }
#define CTRL           { VKKeyKind::Ctrl,      0u, 0u, 0u, 150, "Ctrl"  }
#define ALT            { VKKeyKind::Alt,       0u, 0u, 0u, 150, "Alt"   }
#define AGR            { VKKeyKind::AltGr,     0u, 0u, 0u, 150, "AltGr" }
#define SPC            { VKKeyKind::Space,     0x20u, 0x20u, 0u, 700, "Space" }
#define LEFT           { VKKeyKind::Left,      0u, 0u, 0u, 100, "<-" }
#define RIGHT          { VKKeyKind::Right,     0u, 0u, 0u, 100, "->" }

// =================================================================
//                        US — Standard QWERTY
// =================================================================
static const VKKey us_r0[] = {
  C2('`','~'), C2('1','!'), C2('2','@'), C2('3','#'), C2('4','$'),
  C2('5','%'), C2('6','^'), C2('7','&'), C2('8','*'), C2('9','('),
  C2('0',')'), C2('-','_'), C2('=','+'), BS,
};
static const VKKey us_r1[] = {
  C2('q','Q'), C2('w','W'), C3('e','E',0x20ACu /*€*/), C2('r','R'), C2('t','T'),
  C2('y','Y'), C2('u','U'), C2('i','I'), C2('o','O'), C2('p','P'),
  C2('[','{'), C2(']','}'), C2('\\','|'),
};
static const VKKey us_r2[] = {
  C2('a','A'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'), C2(';',':'),
  C2('\'','"'), ENT,
};
static const VKKey us_r3[] = {
  SHF,
  C2('z','Z'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'), C2('m','M'), C2(',','<'), C2('.','>'), C2('/','?'),
  SHF,
};
static const VKKey us_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow us_rows[] = {
  { us_r0, sizeof(us_r0)/sizeof(us_r0[0]) },
  { us_r1, sizeof(us_r1)/sizeof(us_r1[0]) },
  { us_r2, sizeof(us_r2)/sizeof(us_r2[0]) },
  { us_r3, sizeof(us_r3)/sizeof(us_r3[0]) },
  { us_r4, sizeof(us_r4)/sizeof(us_r4[0]) },
};

// =================================================================
//                        FR — AZERTY (France)
// =================================================================
// Codepoints used:
//   0x00B2 ² | 0x00E9 é | 0x00E8 è | 0x00E7 ç | 0x00E0 à | 0x00F9 ù
//   0x00B0 ° | 0x00B5 µ | 0x00A3 £ | 0x20AC € | 0x00A4 ¤ | 0x00A8 ¨
//   0x005E ^ | 0x00A6 ¦ | 0x00B7 ·
static const VKKey fr_r0[] = {
  C3(0x00B2u, 0x00B2u, 0u),
  C3('&',  '1', 0u),
  C3(0x00E9u, '2', 0x007Eu /*~*/),
  C3('"',  '3', '#'),
  C3('\'', '4', '{'),
  C3('(',  '5', '['),
  C3('-',  '6', '|'),
  C3(0x00E8u, '7', '`'),
  C3('_',  '8', '\\'),
  C3(0x00E7u, '9', 0x005Eu /*^*/),
  C3(0x00E0u, '0', '@'),
  C3(')',  0x00B0u /*°*/, ']'),
  C3('=',  '+', '}'),
  BS,
};
static const VKKey fr_r1[] = {
  C2('a','A'), C2('z','Z'),
  C3('e','E', 0x20ACu /*€*/),
  C2('r','R'), C2('t','T'), C2('y','Y'), C2('u','U'), C2('i','I'),
  C2('o','O'), C2('p','P'),
  C2(0x005Eu /*^*/, 0x00A8u /*¨*/),
  C2('$', 0x00A3u /*£*/),
};
static const VKKey fr_r2[] = {
  C2('q','Q'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'), C2('m','M'),
  C2(0x00F9u /*ù*/, '%'),
  C2('*', 0x00B5u /*µ*/),
  ENT,
};
static const VKKey fr_r3[] = {
  SHF,
  C2('<','>'),
  C2('w','W'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'),
  C2(',','?'),
  C2(';','.'),
  C2(':','/'),
  C2('!', 0x00A7u /*§*/),
  SHF,
};
static const VKKey fr_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow fr_rows[] = {
  { fr_r0, sizeof(fr_r0)/sizeof(fr_r0[0]) },
  { fr_r1, sizeof(fr_r1)/sizeof(fr_r1[0]) },
  { fr_r2, sizeof(fr_r2)/sizeof(fr_r2[0]) },
  { fr_r3, sizeof(fr_r3)/sizeof(fr_r3[0]) },
  { fr_r4, sizeof(fr_r4)/sizeof(fr_r4[0]) },
};

// =================================================================
//                        DE — QWERTZ (Germany)
// =================================================================
// Codepoints: 0x00DF ß | 0x00B4 ´ | 0x00FC ü | 0x00F6 ö | 0x00E4 ä
//             0x00A7 § | 0x00B0 ° | 0x00B2 ² | 0x00B3 ³
static const VKKey de_r0[] = {
  C2(0x005Eu /*^*/, 0x00B0u /*°*/),
  C2('1','!'),
  C3('2','"', 0x00B2u /*²*/),
  C3('3', 0x00A7u /*§*/, 0x00B3u /*³*/),
  C2('4','$'),
  C2('5','%'),
  C2('6','&'),
  C3('7','/', '{'),
  C3('8','(', '['),
  C3('9',')', ']'),
  C3('0','=', '}'),
  C3(0x00DFu /*ß*/, '?', '\\'),
  C2(0x00B4u /*´*/, '`'),
  BS,
};
static const VKKey de_r1[] = {
  C3('q','Q', '@'),
  C2('w','W'),
  C3('e','E', 0x20ACu /*€*/),
  C2('r','R'), C2('t','T'), C2('z','Z'), C2('u','U'), C2('i','I'),
  C2('o','O'), C2('p','P'),
  C2(0x00FCu /*ü*/, 0x00DCu /*Ü*/),
  C3('+','*', '~'),
};
static const VKKey de_r2[] = {
  C2('a','A'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'),
  C2(0x00F6u /*ö*/, 0x00D6u /*Ö*/),
  C2(0x00E4u /*ä*/, 0x00C4u /*Ä*/),
  C2('#','\''),
  ENT,
};
static const VKKey de_r3[] = {
  SHF,
  C3('<','>', '|'),
  C2('y','Y'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'), C2('m','M'),
  C2(',',';'), C2('.',':'), C2('-','_'),
  SHF,
};
static const VKKey de_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow de_rows[] = {
  { de_r0, sizeof(de_r0)/sizeof(de_r0[0]) },
  { de_r1, sizeof(de_r1)/sizeof(de_r1[0]) },
  { de_r2, sizeof(de_r2)/sizeof(de_r2[0]) },
  { de_r3, sizeof(de_r3)/sizeof(de_r3[0]) },
  { de_r4, sizeof(de_r4)/sizeof(de_r4[0]) },
};

// =================================================================
//                        DK — Danish QWERTY
// =================================================================
// Codepoints: 0x00BD ½ | 0x00A7 § | 0x00B4 ´ | 0x00E5 å | 0x00A8 ¨
//             0x00E6 æ | 0x00F8 ø | 0x20AC € | 0x00A3 £
static const VKKey dk_r0[] = {
  C3(0x00BDu /*½*/, 0x00A7u /*§*/, 0u),
  C2('1','!'),
  C3('2','"', '@'),
  C3('3','#', 0x00A3u /*£*/),
  C3('4', 0x00A4u /*¤*/, '$'),
  C2('5','%'),
  C2('6','&'),
  C3('7','/', '{'),
  C3('8','(', '['),
  C3('9',')', ']'),
  C3('0','=', '}'),
  C3('+', '?', 0x00B1u /*±*/),
  C2(0x00B4u /*´*/, '`'),
  BS,
};
static const VKKey dk_r1[] = {
  C2('q','Q'), C2('w','W'),
  C3('e','E', 0x20ACu /*€*/),
  C2('r','R'), C2('t','T'), C2('y','Y'), C2('u','U'), C2('i','I'),
  C2('o','O'), C2('p','P'),
  C2(0x00E5u /*å*/, 0x00C5u /*Å*/),
  C2(0x00A8u /*¨*/, 0x005Eu /*^*/),
};
static const VKKey dk_r2[] = {
  C2('a','A'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'),
  C2(0x00E6u /*æ*/, 0x00C6u /*Æ*/),
  C2(0x00F8u /*ø*/, 0x00D8u /*Ø*/),
  C2('\'','*'),
  ENT,
};
static const VKKey dk_r3[] = {
  SHF,
  C3('<','>', '\\'),
  C2('z','Z'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'), C2('m','M'),
  C2(',',';'), C2('.',':'), C2('-','_'),
  SHF,
};
static const VKKey dk_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow dk_rows[] = {
  { dk_r0, sizeof(dk_r0)/sizeof(dk_r0[0]) },
  { dk_r1, sizeof(dk_r1)/sizeof(dk_r1[0]) },
  { dk_r2, sizeof(dk_r2)/sizeof(dk_r2[0]) },
  { dk_r3, sizeof(dk_r3)/sizeof(dk_r3[0]) },
  { dk_r4, sizeof(dk_r4)/sizeof(dk_r4[0]) },
};

// =================================================================
//                        ES — Spanish QWERTY
// =================================================================
// Codepoints: 0x00BA º | 0x00AA ª | 0x00A1 ¡ | 0x00BF ¿ | 0x00F1 ñ
//             0x00B4 ´ | 0x00A8 ¨ | 0x00E7 ç
static const VKKey es_r0[] = {
  C3(0x00BAu /*º*/, 0x00AAu /*ª*/, '\\'),
  C3('1','!', '|'),
  C3('2','"', '@'),
  C3('3', 0x00B7u /*·*/, '#'),
  C3('4','$', '~'),
  C2('5','%'),
  C3('6','&', 0x00ACu /*¬*/),
  C2('7','/'),
  C2('8','('),
  C2('9',')'),
  C2('0','='),
  C2('\'','?'),
  C3(0x00A1u /*¡*/, 0x00BFu /*¿*/, 0u),
  BS,
};
static const VKKey es_r1[] = {
  C2('q','Q'), C2('w','W'),
  C3('e','E', 0x20ACu /*€*/),
  C2('r','R'), C2('t','T'), C2('y','Y'), C2('u','U'), C2('i','I'),
  C2('o','O'), C2('p','P'),
  C3('`', 0x005Eu /*^*/, '['),
  C3('+', '*', ']'),
};
static const VKKey es_r2[] = {
  C2('a','A'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'),
  C2(0x00F1u /*ñ*/, 0x00D1u /*Ñ*/),
  C3(0x00B4u /*´*/, 0x00A8u /*¨*/, '{'),
  C3(0x00E7u /*ç*/, 0x00C7u /*Ç*/, '}'),
  ENT,
};
static const VKKey es_r3[] = {
  SHF,
  C2('<','>'),
  C2('z','Z'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'), C2('m','M'),
  C2(',',';'), C2('.',':'), C2('-','_'),
  SHF,
};
static const VKKey es_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow es_rows[] = {
  { es_r0, sizeof(es_r0)/sizeof(es_r0[0]) },
  { es_r1, sizeof(es_r1)/sizeof(es_r1[0]) },
  { es_r2, sizeof(es_r2)/sizeof(es_r2[0]) },
  { es_r3, sizeof(es_r3)/sizeof(es_r3[0]) },
  { es_r4, sizeof(es_r4)/sizeof(es_r4[0]) },
};

// =================================================================
//                        IT — Italian QWERTY
// =================================================================
// Codepoints: 0x00EC ì | 0x00E8 è | 0x00E9 é | 0x00F2 ò | 0x00E7 ç
//             0x00E0 à | 0x00B0 ° | 0x00F9 ù | 0x00A7 § | 0x20AC €
static const VKKey it_r0[] = {
  C2('\\','|'),
  C2('1','!'),
  C2('2','"'),
  C3('3', 0x00A3u /*£*/, 0u),
  C2('4','$'),
  C2('5','%'),
  C2('6','&'),
  C2('7','/'),
  C2('8','('),
  C2('9',')'),
  C2('0','='),
  C2('\'','?'),
  C2(0x00ECu /*ì*/, 0x005Eu /*^*/),
  BS,
};
static const VKKey it_r1[] = {
  C2('q','Q'), C2('w','W'),
  C3('e','E', 0x20ACu /*€*/),
  C2('r','R'), C2('t','T'), C2('y','Y'), C2('u','U'), C2('i','I'),
  C2('o','O'), C2('p','P'),
  C3(0x00E8u /*è*/, 0x00E9u /*é*/, '['),
  C3('+', '*', ']'),
};
static const VKKey it_r2[] = {
  C2('a','A'), C2('s','S'), C2('d','D'), C2('f','F'), C2('g','G'),
  C2('h','H'), C2('j','J'), C2('k','K'), C2('l','L'),
  C3(0x00F2u /*ò*/, 0x00E7u /*ç*/, '@'),
  C3(0x00E0u /*à*/, 0x00B0u /*°*/, '#'),
  C3(0x00F9u /*ù*/, 0x00A7u /*§*/, 0u),
  ENT,
};
static const VKKey it_r3[] = {
  SHF,
  C2('<','>'),
  C2('z','Z'), C2('x','X'), C2('c','C'), C2('v','V'), C2('b','B'),
  C2('n','N'), C2('m','M'),
  C2(',',';'), C2('.',':'), C2('-','_'),
  SHF,
};
static const VKKey it_r4[] = {
  CTRL, ALT, SPC, AGR, LEFT, RIGHT,
};
static const VKRow it_rows[] = {
  { it_r0, sizeof(it_r0)/sizeof(it_r0[0]) },
  { it_r1, sizeof(it_r1)/sizeof(it_r1[0]) },
  { it_r2, sizeof(it_r2)/sizeof(it_r2[0]) },
  { it_r3, sizeof(it_r3)/sizeof(it_r3[0]) },
  { it_r4, sizeof(it_r4)/sizeof(it_r4[0]) },
};

// =================================================================
//                        Layout table + lookup
// =================================================================
// Compute the widest row (in units x100) — all our layouts have row 0 == 14.5
// (13 char keys at 1.0 + Backspace at 1.5 = 14.5 units = 1450).
static constexpr int kMaxRowUnitsX100 = 1450;

static const VKLayout sLayouts[] = {
  { "us", us_rows, sizeof(us_rows)/sizeof(us_rows[0]), kMaxRowUnitsX100 },
  { "fr", fr_rows, sizeof(fr_rows)/sizeof(fr_rows[0]), kMaxRowUnitsX100 },
  { "de", de_rows, sizeof(de_rows)/sizeof(de_rows[0]), kMaxRowUnitsX100 },
  { "dk", dk_rows, sizeof(dk_rows)/sizeof(dk_rows[0]), kMaxRowUnitsX100 },
  { "es", es_rows, sizeof(es_rows)/sizeof(es_rows[0]), kMaxRowUnitsX100 },
  { "it", it_rows, sizeof(it_rows)/sizeof(it_rows[0]), kMaxRowUnitsX100 },
};

const VKLayout& VKLayouts::Get(const String& code)
{
  for (const VKLayout& l : sLayouts)
    if (code == l.code) return l;
  return sLayouts[0]; // fallback: US
}
