#pragma once
#include <glad/glad.h>

class GBuffer {
public:
    GLuint fbo = 0;
    GLuint gPosition = 0;
    GLuint gNormal = 0;
    GLuint gAlbedoSpec = 0;
    GLuint rboDepth = 0;

    GBuffer() = default;
    ~GBuffer();

    bool Init(int width, int height);
    void BindForWriting();
    void BindForReading(GLuint startTextureUnit = 0);
    void Resize(int width, int height);
    void CleanUp();

    unsigned int GetFBO() const { return fbo; }
};