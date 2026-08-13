#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 12) out;

uniform mat4 shadowMatrices[4];

void main() {
    for (int cascade = 0; cascade < 4; ++cascade) {
        gl_Layer = cascade; // Assign target slice in GL_TEXTURE_2D_ARRAY
        for (int i = 0; i < 3; ++i) {
            gl_Position = shadowMatrices[cascade] * gl_in[i].gl_Position;
            EmitVertex();
        }
        EndPrimitive();
    }
}