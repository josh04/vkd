#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inTex;

layout (location = 0) out vec2 outTex;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	outTex = inTex;
	gl_Position = vec4(inPos.xy, 0.0, 1.0);
}
