#version 330 core

layout (location = 0) in vec3 aPos; // Position attribute (slot 0)
layout (location = 1) in vec3 aColor; // Color attribute (slot 1)
layout (location = 2) in vec2 aTexCoord; // Texture attribute (slot 2)

out vec3 ourColor; //  Pass this color to the fragment shader
out vec2 TexCoord;

void main()
{
  gl_Position = vec4( aPos, 1.0);
  ourColor = aColor; // Send vertex color down the pipeline
  TexCoord = aTexCoord;
}