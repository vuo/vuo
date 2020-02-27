#version 120

attribute vec3 position;
#define uPosition position;
varying vec2 uUV;

void main()
{
    gl_Position.xyz = uPosition;
    gl_Position.w = 1.0;
}
