#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 screenUV;

void main()
{
    // Maps screen-space coords (-1..1 to 3) into normalized UVs (0..1)
    screenUV = aPos * 0.5 + 0.5;

    // Output z = 1.0 and w = 1.0 to force NDC z/w = 1.0 (Maximum Far Plane Depth)
    gl_Position = vec4(aPos, 1.0, 1.0);
}