#version 450
//
// Alpha-texture fragment shader, used by font atlases.
// Mirrors the GL backends' fragAlphaSrc.
//
// GL sampled a GL_ALPHA texture and read .a. Vulkan has no alpha-only format,
// so the texture is VK_FORMAT_R8_UNORM and the coverage lives in .r instead.
// That single letter is the only difference from the GL shader.
//

layout(set = 0, binding = 0) uniform sampler2D u_sampler;

layout(location = 0) in vec4 v_col;
layout(location = 1) in vec2 v_tex;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = vec4(v_col.rgb, texture(u_sampler, v_tex).r * v_col.a);
}
