#version 450
//
// Colour-only vertex shader. Mirrors the GL backends' vertColorSrc.
//
// The projection and model-view matrices are combined CPU-side into a single
// mat4 pushed as a push constant: two mat4 would be 128 bytes, exactly the
// minimum guaranteed push constant size, leaving no room for anything else.
//

layout(push_constant) uniform Push { mat4 mvp; } push;

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec4 a_col;

layout(location = 0) out vec4 v_col;

void main()
{
  gl_Position = push.mvp * vec4(a_pos, 0.0, 1.0);
  v_col = a_col;
}
