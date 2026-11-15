#version 410 core
layout(location=0) in vec2 pos;
layout(location=1) in vec3 color;
uniform mat4 projection;
out vec3 tint;
void main(){tint=color;gl_Position=projection*vec4(pos,0,1);}
