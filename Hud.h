#pragma once
#include "Shader.h"
#include <vector>
class Hud {
public:
    explicit Hud(const std::string& assets);
    ~Hud();
    void clear();
    void rect(float x,float y,float w,float h,glm::vec3 color);
    void text(float x,float y,const std::string& s,glm::vec3 color={.8f,.87f,.94f},float scale=1.5f);
    void draw(int width,int height);
private:
    Shader shader;
    GLuint vao=0,vbo=0;
    std::vector<float> vertices;
};
