#version 460 core

out vec4 FragColor;

in VS_OUT
{
    vec3 FragPos;
    vec3 Normal;
} fs_in;

void main()
{
    FragColor = vec4(0.25f, 0.25f, 0.25f, 1.0f);
}
