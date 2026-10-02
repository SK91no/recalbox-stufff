#version 450
//
// Textured vertex shader, shared by the RGBA and alpha pipelines.
// Mirrors the GL backends' vertTexSrc.
//

layout(push_constant) uniform Push { mat4 mvp; } push;

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec4 a_col;
layout(location = 2) in vec2 a_tex;

layout(location = 0) out vec4 v_col;
layout(location = 1) out vec2 v_tex;

void main()
{
  gl_Position = push.mvp * vec4(a_pos, 0.0, 1.0);
  v_col = a_col;
  v_tex = a_tex;
}
