#version 330 core

layout(location = 0) in vec3 modelSpaceIn;
layout(location = 1) in vec3 normalIn;
layout(location = 2) in vec2 texCoordIn;

uniform mat4 M;
uniform mat4 VP;

out vec3 normal;
out vec2 texCoord;

void main(){
	vec4 worldPos = M * vec4(modelSpaceIn, 1);
	vec4 MVP = VP * worldPos;

	gl_Position = MVP;
	normal = mat3(M) * normalIn;
	texCoord = texCoordIn;
}