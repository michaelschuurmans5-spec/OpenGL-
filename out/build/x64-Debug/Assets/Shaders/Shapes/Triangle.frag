#version 330 core 

out vec4 FragColor;
in vec3 ourColor; // Received from vertex shader (smoothly interpolated!)
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{

 // Look up the pixel texture coordinate value color map
    FragColor = texture(ourTexture, TexCoord); 

}