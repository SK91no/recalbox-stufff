#version 450
//
// RGBA texture fragment shader. Mirrors the GL backends' fragTexSrc.
//
// The sampled texture is VK_FORMAT_R8G8B8A8_UNORM, which matches the frontend's
// in-memory R,G,B,A byte order exactly — no swizzle needed. See VULKAN.md.
//

layout(set = 0, binding = 0) uniform sampler2D u_sampler;

layout(location = 0) in vec4 v_col;
layout(location = 1) in vec2 v_tex;

layout(location = 0) out vec4 fragColor;

void main()
{
  fragColor = texture(u_sampler, v_tex) * v_col;
}
