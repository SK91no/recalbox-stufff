#version 450

#pragma parameter SCANLINE_BASE_BRIGHTNESS "Scanline Base Brightness" 0.0 0.0 1.0 0.01
#pragma parameter SCANLINE_SINE_COMP_A     "Scanline Sine Comp A"     0.0 0.0 0.10 0.01
#pragma parameter SCANLINE_SINE_COMP_B     "Scanline Sine Comp B"     1.0 0.0 1.0 0.05

#define pi 3.141592654

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float SCANLINE_BASE_BRIGHTNESS;
    float SCANLINE_SINE_COMP_A;
    float SCANLINE_SINE_COMP_B;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 omega;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
    omega       = vec2(pi * params.OutputSize.x, 2.0 * pi * params.SourceSize.y);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 omega;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec2 sine_comp = vec2(params.SCANLINE_SINE_COMP_A, params.SCANLINE_SINE_COMP_B);
    vec3 res       = texture(Source, vTexCoord).rgb;
    vec3 scanline  = res * (params.SCANLINE_BASE_BRIGHTNESS
                            + dot(sine_comp * sin(vTexCoord * omega), vec2(1.0, 1.0)));
    FragColor = vec4(scanline, 1.0);
}
