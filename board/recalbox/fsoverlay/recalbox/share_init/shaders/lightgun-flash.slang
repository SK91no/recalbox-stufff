#version 450

// Lightgun trigger flash + GunCon calibration shader (Vulkan/Slang).
// flash_active: 1.0 during the post-trigger white-flash burst.
// cal_phase:    0 = normal, 1 = capture, 2 = test.
// cal_target_p1/p2: 0..2 = active target (next to shoot), 3 = none.

layout(push_constant) uniform Push
{
   vec4 SourceSize;
   vec4 OriginalSize;
   vec4 OutputSize;
   uint FrameCount;
   float flash_active;
   float cal_phase;
   float cal_player;
   float cal_target_p1;
   float cal_target_p2;
   float cursor_x_p1;
   float cursor_y_p1;
   float cursor_x_p2;
   float cursor_y_p2;
   float cal_y_flip;
} params;

#pragma parameter flash_active   "Flash Active"        0.0 0.0 1.0 1.0
#pragma parameter cal_phase      "GunCon Cal Phase"    0.0 0.0 2.0 1.0
#pragma parameter cal_player     "Cal Player (1..2)"   0.0 0.0 2.0 1.0
#pragma parameter cal_target_p1  "P1 Active Target"    3.0 0.0 3.0 1.0
#pragma parameter cal_target_p2  "P2 Active Target"    3.0 0.0 3.0 1.0
#pragma parameter cursor_x_p1    "P1 Cursor X (UV)"    0.5 0.0 1.0 0.001
#pragma parameter cursor_y_p1    "P1 Cursor Y (UV)"    0.5 0.0 1.0 0.001
#pragma parameter cursor_x_p2    "P2 Cursor X (UV)"    0.5 0.0 1.0 0.001
#pragma parameter cursor_y_p2    "P2 Cursor Y (UV)"    0.5 0.0 1.0 0.001
#pragma parameter cal_y_flip     "Flip Y for cal"      0.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// 5x7 bitmap font (A-Z, 0-9, '(', ')', space). MSB = left column.
const uint FONT[39 * 7] = uint[39 * 7](
    0x0Eu,0x11u,0x11u,0x1Fu,0x11u,0x11u,0x11u, // A
    0x1Eu,0x11u,0x11u,0x1Eu,0x11u,0x11u,0x1Eu, // B
    0x0Eu,0x11u,0x10u,0x10u,0x10u,0x11u,0x0Eu, // C
    0x1Eu,0x11u,0x11u,0x11u,0x11u,0x11u,0x1Eu, // D
    0x1Fu,0x10u,0x10u,0x1Eu,0x10u,0x10u,0x1Fu, // E
    0x1Fu,0x10u,0x10u,0x1Eu,0x10u,0x10u,0x10u, // F
    0x0Eu,0x11u,0x10u,0x17u,0x11u,0x11u,0x0Fu, // G
    0x11u,0x11u,0x11u,0x1Fu,0x11u,0x11u,0x11u, // H
    0x0Eu,0x04u,0x04u,0x04u,0x04u,0x04u,0x0Eu, // I
    0x07u,0x02u,0x02u,0x02u,0x02u,0x12u,0x0Cu, // J
    0x11u,0x12u,0x14u,0x18u,0x14u,0x12u,0x11u, // K
    0x10u,0x10u,0x10u,0x10u,0x10u,0x10u,0x1Fu, // L
    0x11u,0x1Bu,0x15u,0x15u,0x11u,0x11u,0x11u, // M
    0x11u,0x19u,0x15u,0x13u,0x11u,0x11u,0x11u, // N
    0x0Eu,0x11u,0x11u,0x11u,0x11u,0x11u,0x0Eu, // O
    0x1Eu,0x11u,0x11u,0x1Eu,0x10u,0x10u,0x10u, // P
    0x0Eu,0x11u,0x11u,0x11u,0x15u,0x12u,0x0Du, // Q
    0x1Eu,0x11u,0x11u,0x1Eu,0x14u,0x12u,0x11u, // R
    0x0Fu,0x10u,0x10u,0x0Eu,0x01u,0x01u,0x1Eu, // S
    0x1Fu,0x04u,0x04u,0x04u,0x04u,0x04u,0x04u, // T
    0x11u,0x11u,0x11u,0x11u,0x11u,0x11u,0x0Eu, // U
    0x11u,0x11u,0x11u,0x11u,0x11u,0x0Au,0x04u, // V
    0x11u,0x11u,0x11u,0x15u,0x15u,0x1Bu,0x11u, // W
    0x11u,0x11u,0x0Au,0x04u,0x0Au,0x11u,0x11u, // X
    0x11u,0x11u,0x0Au,0x04u,0x04u,0x04u,0x04u, // Y
    0x1Fu,0x01u,0x02u,0x04u,0x08u,0x10u,0x1Fu, // Z
    0x0Eu,0x11u,0x13u,0x15u,0x19u,0x11u,0x0Eu, // 0
    0x04u,0x0Cu,0x04u,0x04u,0x04u,0x04u,0x0Eu, // 1
    0x0Eu,0x11u,0x01u,0x02u,0x04u,0x08u,0x1Fu, // 2
    0x1Fu,0x02u,0x04u,0x02u,0x01u,0x11u,0x0Eu, // 3
    0x02u,0x06u,0x0Au,0x12u,0x1Fu,0x02u,0x02u, // 4
    0x1Fu,0x10u,0x1Eu,0x01u,0x01u,0x11u,0x0Eu, // 5
    0x0Eu,0x10u,0x10u,0x1Eu,0x11u,0x11u,0x0Eu, // 6
    0x1Fu,0x01u,0x02u,0x04u,0x08u,0x08u,0x08u, // 7
    0x0Eu,0x11u,0x11u,0x0Eu,0x11u,0x11u,0x0Eu, // 8
    0x0Eu,0x11u,0x11u,0x0Fu,0x01u,0x01u,0x0Eu, // 9
    0x02u,0x04u,0x08u,0x08u,0x08u,0x04u,0x02u, // (
    0x08u,0x04u,0x02u,0x02u,0x02u,0x04u,0x08u, // )
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u  // space (index 38)
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
   uint mask = FONT[glyph * 7 + row];
   return (mask & (1u << uint(4 - col))) != 0u;
}

// Draws a fixed-size text. `chars[i]` for i ∈ [0..count) is the ASCII code.
// Returns true if (uv) lands on a lit pixel of the text.
bool draw_text(vec2 uv, vec2 origin, vec2 glyph_uv, float spacing_uv, int chars[40], int count)
{
   /* Snap the origin to the pixel grid: half a pixel off and the 5x7 sampling
    * below doubles a glyph row and drops another, which reads as blurry text. */
   vec2 org   = floor(origin * params.OutputSize.xy + 0.5) / params.OutputSize.xy;
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

const vec2 TARGETS[3] = vec2[3](
    vec2(0.06, 0.08),  // 0 visual top-left
    vec2(0.94, 0.08),  // 1 visual top-right
    vec2(0.06, 0.92)   // 2 visual bottom-left
);

bool draw_cursor(vec2 uv, vec2 c, float ar, out vec4 col, vec3 tint)
{
   vec2  d   = uv - c;
   d.x      *= ar;
   float ax  = abs(d.x);
   float ay  = abs(d.y);
   if ((ax < 0.0014 && ay < 0.022) || (ay < 0.0014 && ax < 0.022))
   {
      col = vec4(tint, 1.0);
      return true;
   }
   return false;
}

void main()
{
   if (params.flash_active > 0.5)
   {
      FragColor = vec4(1.0);
      return;
   }

   if (params.cal_phase > 0.5)
   {
      bool  in_phase1 = params.cal_phase < 1.5;
      bool  flipY = params.cal_y_flip > 0.5;
      // Text lives in display space: when the core presents its framebuffer
      // Y-flipped, flip the sampling UV so glyphs read upright (and the
      // blocks land at the right edge), like the targets/cursor below.
      vec2  tuv = flipY ? vec2(vTexCoord.x, 1.0 - vTexCoord.y) : vTexCoord;
      vec3  base = vec3(1.0);
      float ar   = params.OutputSize.x / max(params.OutputSize.y, 1.0);
      int   tp1  = int(params.cal_target_p1 + 0.5);
      int   tp2  = int(params.cal_target_p2 + 0.5);

      // px adapts to vertical resolution: ~2 in 240p, ~2 in 480p (capped).
      float px = max(1.0, params.OutputSize.y / 240.0);
      vec2  gsz = vec2(5.0 * px / params.OutputSize.x, 7.0 * px / params.OutputSize.y);
      float sp  = 1.0 * px / params.OutputSize.x;

      // Title "GUNCON CAL PLAYER n" — top center
      {
         int pdig = 48 + int(params.cal_player + 0.5);
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

      // === Bullseye targets (unchanged from earlier behavior) ===
      const float R_OUTER  = 0.022;
      const float R_BODY   = 0.018;
      const float R_BULLS  = 0.005;
      for (int i = 0; i < 3; i++)
      {
         vec2 t   = TARGETS[i];
         if (flipY) t.y = 1.0 - t.y;
         vec2  d2 = vTexCoord - t;
         d2.x   *= ar;
         float d  = length(d2);
         if (d > R_OUTER)
            continue;
         bool  is_active = in_phase1 && ((i == tp1) || (i == tp2));
         int   smallest_target = (tp1 < tp2) ? tp1 : tp2;
         bool  done = !in_phase1 || (i < smallest_target);
         if (d > R_BODY)
         {
            vec3 col = is_active ? vec3(0.0, 0.0, 0.0)
                     : (done ? vec3(0.0, 0.4, 0.0)
                             : vec3(0.3, 0.3, 0.3));
            FragColor = vec4(col, 1.0);
            return;
         }
         if (d < R_BULLS && (is_active || done))
         {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            return;
         }
         if (is_active)
         {
            FragColor = vec4(1.0, 0.1, 0.1, 1.0);
            return;
         }
         if (done)
         {
            FragColor = vec4(0.1, 0.7, 0.1, 1.0);
            return;
         }
      }

      // Centre "shoot to start" bullseye — phase 2 only. Symmetric, so the
      // Y flip is irrelevant. Shooting it validates the calibration.
      if (!in_phase1)
      {
         vec2  d2 = vTexCoord - vec2(0.5, 0.5);
         d2.x   *= ar;
         float d  = length(d2);
         if (d <= R_OUTER)
         {
            if (d > R_BODY || d < R_BULLS)
            {
               FragColor = vec4(0.0, 0.0, 0.0, 1.0);
               return;
            }
            FragColor = vec4(1.0, 0.1, 0.1, 1.0);
            return;
         }
      }

      // Cursor overlay (phase 2 only)
      if (!in_phase1)
      {
         vec4 cursor_col;
         float cy_p1 = flipY ? 1.0 - params.cursor_y_p1 : params.cursor_y_p1;
         float cy_p2 = flipY ? 1.0 - params.cursor_y_p2 : params.cursor_y_p2;
         if (params.cursor_x_p1 >= 0.0 && draw_cursor(vTexCoord,
                 vec2(params.cursor_x_p1, cy_p1),
                 ar, cursor_col, vec3(0.7, 0.0, 0.0)))
         {
            FragColor = cursor_col;
            return;
         }
         if (params.cursor_x_p2 >= 0.0 && draw_cursor(vTexCoord,
                 vec2(params.cursor_x_p2, cy_p2),
                 ar, cursor_col, vec3(0.0, 0.0, 0.7)))
         {
            FragColor = cursor_col;
            return;
         }
      }
      FragColor = vec4(base, 1.0);
      return;
   }

   FragColor = texture(Source, vTexCoord);
}
