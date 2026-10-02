// Lightgun trigger flash + GunCon calibration shader (GLSL/CG path).

#pragma parameter flash_active   "Flash Active"      0.0 0.0 1.0 1.0
#pragma parameter cal_phase      "GunCon Cal Phase"  0.0 0.0 2.0 1.0
#pragma parameter cal_player     "Cal Player (1..2)" 0.0 0.0 2.0 1.0
#pragma parameter cal_target_p1  "P1 Active Target"  3.0 0.0 3.0 1.0
#pragma parameter cal_target_p2  "P2 Active Target"  3.0 0.0 3.0 1.0
#pragma parameter cursor_x_p1    "P1 Cursor X (UV)"  0.5 0.0 1.0 0.001
#pragma parameter cursor_y_p1    "P1 Cursor Y (UV)"  0.5 0.0 1.0 0.001
#pragma parameter cursor_x_p2    "P2 Cursor X (UV)"  0.5 0.0 1.0 0.001
#pragma parameter cursor_y_p2    "P2 Cursor Y (UV)"  0.5 0.0 1.0 0.001
#pragma parameter cal_y_flip     "Flip Y for cal"    0.0 0.0 1.0 1.0

#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec2 vTexCoord;

uniform mat4 MVPMatrix;

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    vTexCoord = TexCoord.xy;
}

#elif defined(FRAGMENT)

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

uniform sampler2D Texture;
uniform COMPAT_PRECISION vec2 OutputSize;
COMPAT_VARYING vec2 vTexCoord;

#ifdef PARAMETER_UNIFORM
uniform COMPAT_PRECISION float flash_active;
uniform COMPAT_PRECISION float cal_phase;
uniform COMPAT_PRECISION float cal_player;
uniform COMPAT_PRECISION float cal_target_p1;
uniform COMPAT_PRECISION float cal_target_p2;
uniform COMPAT_PRECISION float cursor_x_p1;
uniform COMPAT_PRECISION float cursor_y_p1;
uniform COMPAT_PRECISION float cursor_x_p2;
uniform COMPAT_PRECISION float cursor_y_p2;
uniform COMPAT_PRECISION float cal_y_flip;
#else
#define flash_active   0.0
#define cal_phase      0.0
#define cal_target_p1  3.0
#define cal_target_p2  3.0
#define cursor_x_p1    0.5
#define cursor_y_p1    0.5
#define cursor_x_p2    0.5
#define cursor_y_p2    0.5
#define cal_y_flip     0.0
#endif

bool draw_cursor(vec2 uv, vec2 c, float ar, out vec4 col, vec3 tint)
{
    vec2 d = uv - c;
    d.x   *= ar;
    float ax = abs(d.x);
    float ay = abs(d.y);
    if ((ax < 0.0014 && ay < 0.022) || (ay < 0.0014 && ax < 0.022))
    {
        col = vec4(tint, 1.0);
        return true;
    }
    return false;
}

// 5x7 bitmap font (A-Z, 0-9, '(', ')', space). MSB = left column.
const int FONT[39 * 7] = int[39 * 7](
    0x0E,0x11,0x11,0x1F,0x11,0x11,0x11, // A
    0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E, // B
    0x0E,0x11,0x10,0x10,0x10,0x11,0x0E, // C
    0x1E,0x11,0x11,0x11,0x11,0x11,0x1E, // D
    0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F, // E
    0x1F,0x10,0x10,0x1E,0x10,0x10,0x10, // F
    0x0E,0x11,0x10,0x17,0x11,0x11,0x0F, // G
    0x11,0x11,0x11,0x1F,0x11,0x11,0x11, // H
    0x0E,0x04,0x04,0x04,0x04,0x04,0x0E, // I
    0x07,0x02,0x02,0x02,0x02,0x12,0x0C, // J
    0x11,0x12,0x14,0x18,0x14,0x12,0x11, // K
    0x10,0x10,0x10,0x10,0x10,0x10,0x1F, // L
    0x11,0x1B,0x15,0x15,0x11,0x11,0x11, // M
    0x11,0x19,0x15,0x13,0x11,0x11,0x11, // N
    0x0E,0x11,0x11,0x11,0x11,0x11,0x0E, // O
    0x1E,0x11,0x11,0x1E,0x10,0x10,0x10, // P
    0x0E,0x11,0x11,0x11,0x15,0x12,0x0D, // Q
    0x1E,0x11,0x11,0x1E,0x14,0x12,0x11, // R
    0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E, // S
    0x1F,0x04,0x04,0x04,0x04,0x04,0x04, // T
    0x11,0x11,0x11,0x11,0x11,0x11,0x0E, // U
    0x11,0x11,0x11,0x11,0x11,0x0A,0x04, // V
    0x11,0x11,0x11,0x15,0x15,0x1B,0x11, // W
    0x11,0x11,0x0A,0x04,0x0A,0x11,0x11, // X
    0x11,0x11,0x0A,0x04,0x04,0x04,0x04, // Y
    0x1F,0x01,0x02,0x04,0x08,0x10,0x1F, // Z
    0x0E,0x11,0x13,0x15,0x19,0x11,0x0E, // 0
    0x04,0x0C,0x04,0x04,0x04,0x04,0x0E, // 1
    0x0E,0x11,0x01,0x02,0x04,0x08,0x1F, // 2
    0x1F,0x02,0x04,0x02,0x01,0x11,0x0E, // 3
    0x02,0x06,0x0A,0x12,0x1F,0x02,0x02, // 4
    0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E, // 5
    0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E, // 6
    0x1F,0x01,0x02,0x04,0x08,0x08,0x08, // 7
    0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E, // 8
    0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E, // 9
    0x02,0x04,0x08,0x08,0x08,0x04,0x02, // (
    0x08,0x04,0x02,0x02,0x02,0x04,0x08, // )
    0x00,0x00,0x00,0x00,0x00,0x00,0x00  // space (index 38)
);

// ASCII → glyph index. Returns 38 (space) for unsupported chars.
int char_to_glyph(int c)
{
   if (c >= 65 && c <= 90) return c - 65;       // A-Z
   if (c >= 48 && c <= 57) return 26 + (c - 48); // 0-9
   if (c == 40) return 36; // (
   if (c == 41) return 37; // )
   return 38; // space / unknown
}

bool glyph_pixel_at(int glyph, int col, int row)
{
   if (col < 0 || col > 4 || row < 0 || row > 6) return false;
   int mask = FONT[glyph * 7 + row];
   return (mask & (1 << (4 - col))) != 0;
}

// Draws a fixed-size text. `chars[i]` for i ∈ [0..count) is the ASCII code.
// Returns true if (uv) lands on a lit pixel of the text.
bool draw_text(vec2 uv, vec2 origin, vec2 glyph_uv, float spacing_uv, int chars[40], int count)
{
   // Snap the origin to the pixel grid: half a pixel off and the 5x7 sampling
   // below doubles a glyph row and drops another, which reads as blurry text.
   vec2 org   = floor(origin * OutputSize + 0.5) / OutputSize;
   vec2 local = uv - org;
   if (local.y < 0.0 || local.y >= glyph_uv.y) return false;
   float advance = glyph_uv.x + spacing_uv;
   if (local.x < 0.0 || local.x >= float(count) * advance) return false;
   int slot = int(local.x / advance);
   if (slot < 0 || slot >= count) return false;
   float xrel = local.x - float(slot) * advance;
   if (xrel >= glyph_uv.x) return false;
   int col = int(xrel / glyph_uv.x * 5.0);
   int row = int(local.y / glyph_uv.y * 7.0);
   return glyph_pixel_at(char_to_glyph(chars[slot]), col, row);
}

float text_width_uv(int count, vec2 glyph_uv, float spacing_uv)
{
   return float(count) * (glyph_uv.x + spacing_uv) - spacing_uv;
}


void main()
{
    if (flash_active > 0.5)
    {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    if (cal_phase > 0.5)
    {
        bool  in_phase1 = cal_phase < 1.5;
        // Both phases pure white so the GunCon photodiode auto-cals
        // immediately on phase 1.
        vec3  base = vec3(1.0);

        bool  flipYtxt = cal_y_flip > 0.5;
        vec2  tuv = flipYtxt ? vec2(vTexCoord.x, 1.0 - vTexCoord.y) : vTexCoord;
        float ar   = OutputSize.x / max(OutputSize.y, 1.0);
        int   tp1  = int(cal_target_p1 + 0.5);
        int   tp2  = int(cal_target_p2 + 0.5);

          // px adapts to vertical resolution: ~2 in 240p, ~2 in 480p (capped).
          float px = max(1.0, OutputSize.y / 240.0);
          vec2  gsz = vec2(5.0 * px / OutputSize.x, 7.0 * px / OutputSize.y);
          float sp  = 1.0 * px / OutputSize.x;

          // Title "GUNCON CAL PLAYER n" — top center
          {
             int pdig = 48 + int(cal_player + 0.5);
             int t[40] = int[40](71,85,78,67,79,78,32,67,65,76,32,80,76,65,89,69,82,32,pdig,
                                  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
             float w = text_width_uv(19, gsz, sp);
             vec2 o = vec2(0.5 - w * 0.5, 0.22);
             if (draw_text(tuv, o, gsz, sp, t, 19)) { FragColor = vec4(0.0,0.0,0.0,1.0); return; }
          }

          if (in_phase1)
          {
             // "SHOOT THE RED TARGET (n LEFT)" — 29 chars, drawn in one go: three
             // segments would land their origins between pixels and blur the text.
             int active_target = (tp1 <= 2) ? tp1 : ((tp2 <= 2) ? tp2 : 0);
             int digit_char = 48 + (3 - active_target);
             int a[40] = int[40](83,72,79,79,84,32,84,72,69,32,82,69,68,32,84,65,82,71,69,84,32,40,digit_char,32,76,69,70,84,41,
                                  0,0,0,0,0,0,0,0,0,0,0);
             float wa = text_width_uv(29, gsz, sp);
             vec2 oa = vec2(0.5 - wa * 0.5, 0.40);
             if (draw_text(tuv, oa, gsz, sp, a, 29)) { FragColor = vec4(0.0,0.0,0.0,1.0); return; }
          }
          else
          {
             // "SHOOT HERE TO START" — 19 chars. The centre bullseye below is
             // the validate button: shooting is the only input guaranteed to
             // reach the gun's port.
             int a[40] = int[40](83,72,79,79,84,32,72,69,82,69,32,84,79,32,83,84,65,82,84,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
             float wa = text_width_uv(19, gsz, sp);
             vec2 oa = vec2(0.5 - wa * 0.5, 0.34);
             if (draw_text(tuv, oa, gsz, sp, a, 19)) { FragColor = vec4(0.0,0.0,0.0,1.0); return; }

             // "A OR B TO RETRY" — 15 chars, below the bullseye
             int s[40] = int[40](65,32,79,82,32,66,32,84,79,32,82,69,84,82,89,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
             float ws = text_width_uv(15, gsz, sp);
             vec2 os = vec2(0.5 - ws * 0.5, 0.62);
             if (draw_text(tuv, os, gsz, sp, s, 15)) { FragColor = vec4(0.0,0.0,0.0,1.0); return; }
          }

        // 3 corners enough for linear bbox mapping.
        vec2 targets[3];
        targets[0] = vec2(0.06, 0.08);  // visual top-left
        targets[1] = vec2(0.94, 0.08);  // visual top-right
        targets[2] = vec2(0.06, 0.92);  // visual bottom-left

        bool flipY = cal_y_flip > 0.5;
        for (int i = 0; i < 3; i++)
        {
            vec2 t   = targets[i];
            if (flipY) t.y = 1.0 - t.y;
            vec2  d2 = vTexCoord - t;
            d2.x   *= ar;
            float d  = length(d2);
            bool  is_active = in_phase1 && ((i == tp1) || (i == tp2));
            int   smallest_target = (tp1 < tp2) ? tp1 : tp2;
            bool  done = !in_phase1 || (i < smallest_target);
            if (d < 0.014)
            {
                if (is_active) { FragColor = vec4(1.0, 0.1, 0.1, 1.0); return; }
                if (done)      { FragColor = vec4(0.1, 0.7, 0.1, 1.0); return; }
            }
            if (d < 0.020 && d > 0.014)
            {
                vec3 col = is_active ? vec3(0.0, 0.0, 0.0)
                         : (done ? vec3(0.0, 0.4, 0.0)
                                 : vec3(0.3, 0.3, 0.3));
                FragColor = vec4(col, 1.0);
                return;
            }
        }

        if (!in_phase1)
        {
            vec4 cursor_col;
            float cy_p1 = flipY ? 1.0 - cursor_y_p1 : cursor_y_p1;
            float cy_p2 = flipY ? 1.0 - cursor_y_p2 : cursor_y_p2;
            if (cursor_x_p1 >= 0.0 && draw_cursor(vTexCoord,
                    vec2(cursor_x_p1, cy_p1),
                    ar, cursor_col, vec3(0.7, 0.0, 0.0)))
            {
                FragColor = cursor_col;
                return;
            }
            if (cursor_x_p2 >= 0.0 && draw_cursor(vTexCoord,
                    vec2(cursor_x_p2, cy_p2),
                    ar, cursor_col, vec3(0.0, 0.0, 0.7)))
            {
                FragColor = cursor_col;
                return;
            }
        }
        FragColor = vec4(base, 1.0);
        return;
    }

    FragColor = COMPAT_TEXTURE(Texture, vTexCoord);
}

#endif
