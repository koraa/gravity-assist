#version 330 core

in  vec3 tex_cords;
out vec4 color;

uniform samplerCube tex;

void main() {
	color = texture(tex, tex_cords);
}
