#version 450

#extension GL_GOOGLE_include_directive : enable

#include "../include/srgb.h"

layout (binding = 0) uniform sampler2D imageSampler;

layout (location = 0) in vec2 inTex;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(imageSampler, inTex);
}