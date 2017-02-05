#version 330 core

layout(location = 0) in vec3 pos;
out vec3 tex_cords;

uniform mat4 mvp;

void main(){
  gl_Position = mvp * vec4(pos, 1);
  tex_cords = pos;
}

