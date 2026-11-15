#include "Hud.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cctype>
#include <map>
#include <stdexcept>

// Original compact 5x7 bitmap alphabet; each entry is a row bitmask.
static const std::map<char,std::array<int,7>> glyphs={
{'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},
{'C',{14,17,16,16,16,17,14}},{'D',{30,17,17,17,17,17,30}},
{'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
{'G',{14,17,16,23,17,17,15}},{'H',{17,17,17,31,17,17,17}},
{'I',{14,4,4,4,4,4,14}},{'J',{7,2,2,2,18,18,12}},
{'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
{'M',{17,27,21,21,17,17,17}},{'N',{17,25,25,21,19,19,17}},
{'O',{14,17,17,17,17,17,14}},{'P',{30,17,17,30,16,16,16}},
{'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
{'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},
{'U',{17,17,17,17,17,17,14}},{'V',{17,17,17,17,17,10,4}},
{'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
{'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
{'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},
{'2',{14,17,1,2,4,8,31}},{'3',{30,1,1,14,1,1,30}},
{'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
{'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},
{'8',{14,17,17,14,17,17,14}},{'9',{14,17,17,15,1,1,14}},
{'.',{0,0,0,0,0,6,6}},{':',{0,6,6,0,6,6,0}},
{'-',{0,0,0,31,0,0,0}},{'+',{0,4,4,31,4,4,0}},
{'/',{1,2,2,4,8,8,16}},{'=',{0,0,31,0,31,0,0}},
{'(',{2,4,8,8,8,4,2}},{')',{8,4,2,2,2,4,8}},
{'[',{14,8,8,8,8,8,14}},{']',{14,2,2,2,2,2,14}},
{'<',{2,4,8,16,8,4,2}},{'>',{8,4,2,1,2,4,8}},
{'?',{14,17,1,2,4,0,4}},{'_', {0,0,0,0,0,0,31}},
{'%',{17,2,4,8,16,17,0}},{',',{0,0,0,0,6,4,8}}
};
Hud::Hud(const std::string& assets):shader((assets+"/shaders/hud.vert").c_str(),(assets+"/shaders/hud.frag").c_str()){
    if(!shader.ID) throw std::runtime_error("HUD shader failed");
    glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,5*sizeof(float),nullptr); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(2*sizeof(float))); glEnableVertexAttribArray(1);
}
Hud::~Hud(){glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(shader.ID);}
void Hud::clear(){vertices.clear();}
void Hud::rect(float x,float y,float w,float h,glm::vec3 c){
    for(auto p: {glm::vec2(x,y),glm::vec2(x+w,y),glm::vec2(x+w,y+h),glm::vec2(x,y),glm::vec2(x+w,y+h),glm::vec2(x,y+h)}){
        vertices.insert(vertices.end(),{p.x,p.y,c.r,c.g,c.b});
    }
}
void Hud::text(float x,float y,const std::string& s,glm::vec3 c,float scale){
    for(unsigned char ch:s){
        auto it=glyphs.find(char(std::toupper(ch)));
        if(it!=glyphs.end()) for(int row=0;row<7;++row) for(int col=0;col<5;++col)
            if(it->second[row] & (1<<(4-col))) rect(x+col*scale,y+row*scale,scale,scale,c);
        x+=6*scale;
    }
}
void Hud::draw(int w,int h){
    shader.use();shader.setMat4("projection",glm::ortho(0.f,float(w),float(h),0.f));
    glDisable(GL_DEPTH_TEST);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(float),vertices.data(),GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES,0,GLsizei(vertices.size()/5));glBindVertexArray(0);glEnable(GL_DEPTH_TEST);
}
