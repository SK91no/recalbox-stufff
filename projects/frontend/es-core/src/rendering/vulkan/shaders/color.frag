#version 450
//
// Colour-only fragment shader. Mirrors the GL backends' fragColorSrc.
//

layout(location = 0) in vec4 v_col;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = v_col;
}
