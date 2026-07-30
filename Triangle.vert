
#version 330 core

layout (location = 0) in vec3 aPos; // Bind to index 0 using array buffer 

void main()
{
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);  // Points of Triangle 
}