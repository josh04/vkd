#version 450

layout (binding = 0) uniform sampler2D samplerColorMap;
layout (binding = 1) uniform sampler2D samplerGradientRamp;

layout (location = 0) in vec4 inColor;
layout (location = 1) in float inGradientPos;

layout (location = 0) out vec4 outFragColor;

void main () 
{
	vec3 color = texture(samplerGradientRamp, vec2(inGradientPos, 0.0)).rgb;
	vec4 inten = texture(samplerColorMap, gl_PointCoord);
	outFragColor = inten * vec4(color, 1.0);
	outFragColor.w = pow((inten.r + inten.g + inten.b)/3.0, 2.2);
}
