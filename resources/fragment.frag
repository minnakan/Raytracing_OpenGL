#version 330 core

layout (location = 0) out vec4 FragColor;
in vec3 Normal;

void main()
{
	vec3 normalColor = normalize(Normal) * 0.5 + 0.5;
	FragColor = vec4(normalColor, 1.0);
	//FragColor = vec4(1,0,1,1);

}