#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <chrono>
#include <GLFW/glfw3.h>
#include <mach-o/dyld.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include "Animation.h"
#include "Shader.h"
#include "Texture.h"
#include "Hud.h"

namespace fs=std::filesystem;
constexpr int sidebar=350, footer=112;
const glm::vec3 accent(.35f,.85f,.78f), muted(.48f,.59f,.69f);
std::string fixed(double v,int digits=3){std::ostringstream s;s<<std::fixed<<std::setprecision(digits)<<v;return s.str();}
std::string scientific(double v){std::ostringstream s;s<<std::scientific<<std::setprecision(3)<<v;return s.str();}
struct Mesh {
    GLuint vao=0,vbo=0; GLsizei count=0; int stride;
    explicit Mesh(int s=3):stride(s){
        glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,s*sizeof(float),nullptr);glEnableVertexAttribArray(0);
        if(s==5){glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,s*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);}
    }
    Mesh(const Mesh&)=delete; Mesh& operator=(const Mesh&)=delete;
    ~Mesh(){glDeleteVertexArrays(1,&vao);glDeleteBuffers(1,&vbo);}
    void upload(const std::vector<float>& v,GLenum usage=GL_STATIC_DRAW){
        count=GLsizei(v.size()/stride);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(float),v.data(),usage);
    }
    void draw(GLenum mode,int start=0,int n=-1){glBindVertexArray(vao);glDrawArrays(mode,start,n<0?count:n);}
};
struct App {
    std::vector<Planet> planets=createPlanets();
    astro::SimulationClock clock;
    astro::DisplayScale scale;
    int selected=3;
    bool follow=false,orbits=true,axes=true,labels=true,drag=false,editingDate=false;
    double yaw=0,pitch=35,distance=46,lastX=0,lastY=0;
    std::string dateInput,status;
    std::array<glm::dvec2,9> screen{};
    std::array<bool,9> visible{};
    void focus(int index){selected=index;follow=true;distance=std::max(scale.radius(planets[index])*8,0.025);}
    void reset(bool all=false){follow=false;distance=(all?90:3.85)*scale.unitsPerAU;yaw=0;pitch=35;}
    void toggleScale(){double before=scale.radius(planets[selected]);scale.trueScale=!scale.trueScale;
        if(follow)distance*=scale.radius(planets[selected])/before;}
};
App& app(GLFWwindow* w){return *static_cast<App*>(glfwGetWindowUserPointer(w));}
void applyDate(App& a){
    try{
        if(a.dateInput.size()!=10 || a.dateInput[4]!='-' || a.dateInput[7]!='-')throw std::runtime_error("format");
        for(size_t i=0;i<10;++i)if(i!=4 && i!=7 && (a.dateInput[i]<'0'||a.dateInput[i]>'9'))throw std::runtime_error("digits");
        int y=std::stoi(a.dateInput.substr(0,4)),m=std::stoi(a.dateInput.substr(5,2)),d=std::stoi(a.dateInput.substr(8,2));
        a.clock.setDate(astro::julianDate(y,m,d));a.clock.paused=true;a.editingDate=false;a.status="DATE SET - TDB, NOT LOCAL TIME";
    }catch(const std::exception&){a.status="USE VALID YYYY-MM-DD (1800-2050)";}
}
void key(GLFWwindow* w,int k,int,int action,int){
    auto& a=app(w);
    if(action!=GLFW_PRESS && action!=GLFW_REPEAT)return;
    if(a.editingDate){
        if(k==GLFW_KEY_ESCAPE){a.editingDate=false;a.status.clear();}
        else if(k==GLFW_KEY_ENTER)applyDate(a);
        else if(k==GLFW_KEY_BACKSPACE && !a.dateInput.empty())a.dateInput.pop_back();
        return;
    }
    if(action!=GLFW_PRESS)return;
    if(k>=GLFW_KEY_0 && k<=GLFW_KEY_8)a.focus(k-GLFW_KEY_0);
    switch(k){
        case GLFW_KEY_ESCAPE:glfwSetWindowShouldClose(w,true);break;
        case GLFW_KEY_SPACE:a.clock.paused=!a.clock.paused;break;
        case GLFW_KEY_EQUAL:case GLFW_KEY_KP_ADD:a.clock.daysPerSecond=std::copysign(std::min(std::abs(a.clock.daysPerSecond)*2,4096.0),a.clock.daysPerSecond);break;
        case GLFW_KEY_MINUS:case GLFW_KEY_KP_SUBTRACT:a.clock.daysPerSecond=std::copysign(std::max(std::abs(a.clock.daysPerSecond)/2,1.0/86400),a.clock.daysPerSecond);break;
        case GLFW_KEY_B:a.clock.daysPerSecond=-a.clock.daysPerSecond;break;
        case GLFW_KEY_LEFT_BRACKET:a.clock.paused=true;a.clock.step(-1);break;
        case GLFW_KEY_RIGHT_BRACKET:a.clock.paused=true;a.clock.step(1);break;
        case GLFW_KEY_J:a.clock.setDate(astro::j2000);a.clock.paused=true;break;
        case GLFW_KEY_T:a.editingDate=true;a.dateInput.clear();a.clock.paused=true;a.status="TYPE YYYY-MM-DD THEN ENTER";break;
        case GLFW_KEY_F:a.focus(a.selected);break;
        case GLFW_KEY_R:a.reset();break;
        case GLFW_KEY_V:a.reset(true);break;
        case GLFW_KEY_O:a.orbits=!a.orbits;break;
        case GLFW_KEY_X:a.axes=!a.axes;break;
        case GLFW_KEY_L:a.labels=!a.labels;break;
        case GLFW_KEY_M:a.toggleScale();break;
        case GLFW_KEY_COMMA:case GLFW_KEY_PERIOD:{
            double old=a.scale.radiusExaggeration;
            a.scale.radiusExaggeration=std::clamp(old*(k==GLFW_KEY_PERIOD?1.25:.8),1.0,1000.0);
            if(a.follow && !a.scale.trueScale)a.distance*=a.scale.radiusExaggeration/old;
            break;}
        case GLFW_KEY_K:case GLFW_KEY_P:{
            double old=a.scale.unitsPerAU;
            a.scale.unitsPerAU=std::clamp(old*(k==GLFW_KEY_P?1.25:.8),1.0,100.0);
            a.distance*=a.scale.unitsPerAU/old;break;}
    }
}
void character(GLFWwindow* w,unsigned int c){auto& a=app(w);if(a.editingDate && a.dateInput.size()<10 && ((c>='0'&&c<='9')||c=='-'))a.dateInput+=char(c);}
void mouse(GLFWwindow* w,int button,int action,int){
    auto& a=app(w);double x,y;glfwGetCursorPos(w,&x,&y);int width,height;glfwGetWindowSize(w,&width,&height);
    if(button==GLFW_MOUSE_BUTTON_RIGHT){a.drag=action==GLFW_PRESS && x>sidebar && y<height-footer;a.lastX=x;a.lastY=y;return;}
    if(button!=GLFW_MOUSE_BUTTON_LEFT || action!=GLFW_PRESS || a.editingDate)return;
    if(x>=16 && x<334 && y>=436 && y<586){
        int col=x>=175?1:0,row=int((y-436)/30),index=row*2+col;
        if(index<9)a.focus(index);return;
    }
    if(x>sidebar && y>=height-footer+16 && y<height-footer+44){
        double bx=x-sidebar-20;
        if(bx>=0 && bx<100)a.clock.paused=!a.clock.paused;
        else if(bx>=110 && bx<210){a.clock.paused=true;a.clock.step(-1);}
        else if(bx>=220 && bx<320){a.clock.paused=true;a.clock.step(1);}
        else if(bx>=330 && bx<430)a.toggleScale();
        return;
    }
    if(x<=sidebar || y>=height-footer)return;
    double best=18;int pick=-1;
    for(int i=0;i<9;++i)if(a.visible[i]){double d=glm::length(a.screen[i]-glm::dvec2(x,y));if(d<best){best=d;pick=i;}}
    if(pick>=0)a.focus(pick);
}
void cursor(GLFWwindow* w,double x,double y){auto& a=app(w);if(a.drag){a.yaw-=(x-a.lastX)*.3;a.pitch=std::clamp(a.pitch+(y-a.lastY)*.3,-85.0,85.0);}a.lastX=x;a.lastY=y;}
void scroll(GLFWwindow* w,double,double y){auto& a=app(w);if(!a.editingDate)a.distance*=std::exp(-y*.12);}
std::string assetDirectory(){
    uint32_t size=0;_NSGetExecutablePath(nullptr,&size);std::vector<char> buffer(size);
    if(_NSGetExecutablePath(buffer.data(),&size)!=0)throw std::runtime_error("Cannot locate executable");
    return fs::weakly_canonical(buffer.data()).parent_path().string();
}
void capture(const std::string& file,int w,int h,bool offscreen){
    std::vector<unsigned char> pixels(size_t(w)*h*3);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadBuffer(offscreen?GL_COLOR_ATTACHMENT0:GL_BACK);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream out(file,std::ios::binary);if(!out)throw std::runtime_error("Cannot write screenshot");
    out<<"P6\n"<<w<<" "<<h<<"\n255\n";
    for(int y=h-1;y>=0;--y)out.write(reinterpret_cast<const char*>(pixels.data()+size_t(y)*w*3),w*3);
}
void exportCSV(const App& a,const std::string& file){
    std::ofstream f(file);if(!f)throw std::runtime_error("Cannot write CSV");
    f<<"name,jd_tdb,x_ecliptic_au,y_ecliptic_au,z_ecliptic_au,r_au,speed_two_body_km_s\n"<<std::setprecision(15);
    for(const auto& p:a.planets){auto v=astro::positionAU(p,a.clock.jd);f<<p.name<<','<<a.clock.jd<<','<<v.x<<','<<v.y<<','<<v.z<<','<<glm::length(v)<<','<<astro::speedKmS(p,a.clock.jd)<<'\n';}
}
void panel(Hud& hud,const App& a,int width,int height){
    hud.rect(0,0,sidebar,height,{.035f,.055f,.085f});
    hud.rect(sidebar,0,1,height,{.12f,.2f,.25f});
    hud.text(18,20,"SOLAR SYSTEM",accent,2.7f);
    hud.text(18,48,"SCIENTIFIC EXPLORER / JPL MODEL",muted,1.4f);
    hud.rect(18,73,314,1,{.16f,.24f,.3f});
    const auto& p=a.planets[a.selected];auto e=astro::elementsAt(p,a.clock.jd);
    hud.text(18,89,p.name,p.color,2.6f);
    hud.text(18,117,a.follow?"CAMERA TRACKING BODY":"CAMERA: SYSTEM VIEW",muted);
    auto line=[&](int y,const std::string& s){hud.text(18,float(y),s);};
    line(144,"RADIUS KM       "+fixed(p.radiusKm,1));
    line(165,"MASS KG         "+scientific(p.massKg));
    line(186,"ROTATION H      "+fixed(p.rotationHours,1));
    line(207,"OBLIQUITY DEG   "+fixed(p.obliquityDeg,3));
    if(a.selected){
        line(235,"A AU            "+fixed(e.a,6));
        line(256,"ECCENTRICITY    "+fixed(e.e,6));
        line(277,"ORBIT TILT DEG  "+fixed(e.inclination,5));
        line(298,"DISTANCE AU     "+fixed(glm::length(astro::positionAU(p,a.clock.jd)),6));
        line(319,"SPEED KM/S      "+fixed(astro::speedKmS(p,a.clock.jd),3));
        line(340,"PERIOD DAYS     "+fixed(astro::periodDays(p,a.clock.jd),2));
        line(361,"PERI / APHELION AU");
        line(382,fixed(e.a*(1-e.e),5)+" / "+fixed(e.a*(1+e.e),5));
    }else{line(242,"HELIOCENTRIC ORIGIN");line(264,"RADIUS: VOLUMETRIC MEAN");line(286,"ROTATION: ADOPTED 16 DEG LAT.");}
    hud.text(18,415,"SELECT / TRACK A BODY",accent);
    for(int i=0;i<9;++i){float x=18+(i%2)*157,y=436+(i/2)*30;
        hud.rect(x,y,150,25,i==a.selected?glm::vec3(.12f,.25f,.3f):glm::vec3(.065f,.1f,.14f));
        hud.text(x+8,y+7,std::to_string(i)+" "+a.planets[i].name,a.planets[i].color,1.4f);
    }
    hud.text(18,600,"MODEL NOTES",accent);
    hud.text(18,622,"JPL TABLE 1: 1800-2050 TDB",muted,1.3f);
    hud.text(18,643,"EARTH POSITION = EARTH-MOON BARY.",muted,1.3f);
    hud.text(18,664,"POLE AZIMUTH / SPIN PHASE: DEMO",muted,1.3f);
    hud.text(18,685,"SPEED / PERIOD: TWO-BODY ESTIMATE",muted,1.3f);
    hud.text(18,706,"STARS / RINGS: ILLUSTRATIONS",muted,1.3f);
    hud.text(18,727,"SOURCES: DOCS/SCIENCE.MD",muted,1.3f);
    hud.text(sidebar+20,20,astro::calendarDate(a.clock.jd),accent,1.8f);
    hud.text(sidebar+20,43,"JD "+fixed(a.clock.jd,5)+"   "+fixed(a.clock.daysPerSecond,5)+" DAYS/S"+(a.clock.paused?"   PAUSED":"   RUNNING"),muted,1.4f);
    hud.text(sidebar+20,65,a.scale.trueScale?"TRUE SIZE / DISTANCE SCALE":"DISPLAY RADII EXAGGERATED - NOT TO SCALE",{.94f,.71f,.36f},1.4f);
    hud.text(sidebar+20,86,"UNITS/AU "+fixed(a.scale.unitsPerAU,2)+"  PLANET X"+fixed(a.scale.trueScale?1:a.scale.radiusExaggeration,0)+"  SUN X"+fixed(a.scale.trueScale?1:a.scale.radiusExaggeration/50,1),muted,1.3f);
    if(a.editingDate)hud.text(sidebar+20,112,"DATE: "+a.dateInput+"_",accent,2);
    if(!a.status.empty())hud.text(sidebar+20,138,a.status,{.94f,.71f,.36f},1.3f);
    if(a.clock.jd==astro::minJD || a.clock.jd==astro::maxJD)hud.text(sidebar+20,162,"MODEL DATE LIMIT REACHED - PAUSED",{1,.4f,.3f});
    float y=float(height-footer);hud.rect(sidebar,y,width-sidebar,footer,{.035f,.055f,.085f});
    std::array<std::string,4> buttons={a.clock.paused?"PLAY":"PAUSE","-1 DAY","+1 DAY","SCALE [M]"};
    for(int i=0;i<4;++i){hud.rect(sidebar+20+i*110,y+16,100,28,{.1f,.18f,.23f});hud.text(sidebar+29+i*110,y+25,buttons[i],accent,1.3f);}
    hud.text(sidebar+20,y+58,"RMB DRAG: ORBIT  SCROLL/W/S: ZOOM  F: TRACK  R: INNER  V: ALL",muted,1.2f);
    hud.text(sidebar+20,y+77,"SPACE: PAUSE  +/-: RATE  B: REVERSE  [/]: DAY  T: DATE  J: J2000",muted,1.2f);
    hud.text(sidebar+20,y+96,"O: ORBITS  X: AXES  L: LABELS  ,/.: RADII  K/P: DISTANCE SCALE",muted,1.2f);
}
int run(GLFWwindow* window,App& a,const std::string& shot,int frameLimit){
    std::string assets=assetDirectory();
    Shader shader((assets+"/shaders/vertex.glsl").c_str(),(assets+"/shaders/fragment.glsl").c_str());
    Shader line((assets+"/shaders/line.vert").c_str(),(assets+"/shaders/line.frag").c_str());
    if(!shader.ID || !line.ID)throw std::runtime_error("Scene shader failure");
    struct Cleanup {GLuint p,q;std::vector<GLuint> textures;~Cleanup(){for(auto t:textures)glDeleteTextures(1,&t);glDeleteProgram(p);glDeleteProgram(q);}} resources{shader.ID,line.ID,{}};
    for(const auto& p:a.planets){GLuint t=loadTexture(assets+"/textures/"+p.texture);if(!t)throw std::runtime_error("Missing texture: "+p.texture);resources.textures.push_back(t);}
    Hud hud(assets);Mesh sphere(5),orbits,stars,rings,axis;
    sphere.upload(createSphere(64,32));axis.upload({0,-1.6f,0,0,1.6f,0});
    std::vector<float> v;std::mt19937 random(42);std::uniform_real_distribution<float> unit(0,1);
    for(int i=0;i<800;++i){double y=2*unit(random)-1,phi=2*astro::pi*unit(random),r=std::sqrt(1-y*y);v.insert(v.end(),{float(r*std::cos(phi)*1500),float(y*1500),float(r*std::sin(phi)*1500)});}stars.upload(v);v.clear();
    for(int j=0;j<3;++j)for(int i=0;i<180;++i){double t=2*astro::pi*i/180,r=1.5+.25*j;v.insert(v.end(),{float(r*std::cos(t)),0,float(r*std::sin(t))});}rings.upload(v);
    if(window){
    glfwSetWindowUserPointer(window,&a);glfwSetKeyCallback(window,key);glfwSetCharCallback(window,character);
    glfwSetMouseButtonCallback(window,mouse);glfwSetCursorPosCallback(window,cursor);glfwSetScrollCallback(window,scroll);
    }
    glEnable(GL_DEPTH_TEST);glEnable(GL_MULTISAMPLE);
    std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<"\nRenderer "<<glGetString(GL_RENDERER)<<"\nAssets "<<assets<<'\n';
    auto timer=[](){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();};
    double last=timer();int frames=0;
    while(!window || !glfwWindowShouldClose(window)){
        if(window)glfwPollEvents();
        double now=timer(),dt=now-last;last=now;
        // Intentional suspend policy: freeze while unfocused; discard stalls >0.5 s.
        if(window && glfwGetWindowAttrib(window,GLFW_FOCUSED) && dt<=.5)a.clock.advance(dt);
        int fw=1280,fh=800,w=1280,h=800;
        if(window){glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&w,&h);}
        if(fw==0 || fh==0){glfwWaitEventsTimeout(.05);continue;}
        if(window && !a.editingDate){
            double t=std::min(dt,.1);
            if(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)a.distance*=std::exp(-t);
            if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS)a.distance*=std::exp(t);
            if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS)a.yaw-=t*60;
            if(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS)a.yaw+=t*60;
            if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)a.pitch+=t*60;
            if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)a.pitch-=t*60;
        }
        a.pitch=std::clamp(a.pitch,-85.0,85.0);
        double minDistance=a.follow?a.scale.radius(a.planets[a.selected])*2.5:.1;
        a.distance=std::clamp(a.distance,minDistance,200*a.scale.unitsPerAU);
        glm::dvec3 target=a.follow?a.scale.position(a.planets[a.selected],a.clock.jd):glm::dvec3(0);
        double yaw=astro::radians(a.yaw),pitch=astro::radians(a.pitch);
        glm::dvec3 eye=target+a.distance*glm::dvec3(std::cos(pitch)*std::sin(yaw),std::sin(pitch),std::cos(pitch)*std::cos(yaw));
        // Floating origin: subtract camera target in double precision before casting.
        glm::mat4 view=glm::lookAt(glm::vec3(eye-target),glm::vec3(0),glm::vec3(0,1,0));
        float nearPlane=float(std::max(1e-7,a.distance*.0005));
        glm::mat4 projection=glm::perspective(glm::radians(45.f),float(w-sidebar)/float(h-footer),nearPlane,float(3000+200*a.scale.unitsPerAU));
        glViewport(0,0,fw,fh);glClearColor(.008f,.014f,.027f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        int vx=int(double(sidebar)*fw/w),vy=int(double(footer)*fh/h);
        glViewport(vx,vy,fw-vx,fh-vy);
        line.use();line.setMat4("projection",projection);line.setMat4("view",glm::mat4(glm::mat3(view)));line.setMat4("model",glm::mat4(1));line.setVec3("lineColor",{.4f,.48f,.58f});
        glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_PROGRAM_POINT_SIZE);stars.draw(GL_POINTS);glDisable(GL_PROGRAM_POINT_SIZE);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
        line.setMat4("view",view);
        if(a.orbits){
            v.clear();for(int i=1;i<9;++i){auto e=astro::elementsAt(a.planets[i],a.clock.jd);for(int j=0;j<360;++j){auto pos=astro::toScene(astro::orbitPoint(e,2*astro::pi*j/360))*a.scale.unitsPerAU-target;v.insert(v.end(),{float(pos.x),float(pos.y),float(pos.z)});}}
            orbits.upload(v,GL_STREAM_DRAW);
            for(int i=1;i<9;++i){line.setVec3("lineColor",a.planets[i].color*(i==a.selected?.65f:.24f));orbits.draw(GL_LINE_LOOP,(i-1)*360,360);}
        }
        shader.use();shader.setMat4("projection",projection);shader.setMat4("view",view);
        shader.setVec3("lightPos",glm::vec3(-target));shader.setVec3("viewPos",glm::vec3(eye-target));glUniform1i(glGetUniformLocation(shader.ID,"planetTexture"),0);
        a.visible.fill(false);
        for(int i=0;i<9;++i){
            const auto& p=a.planets[i];glm::dvec3 pos=a.scale.position(p,a.clock.jd)-target;double r=a.scale.radius(p);
            glm::mat4 model=glm::translate(glm::mat4(1),glm::vec3(pos))*glm::mat4(astro::axisOrientation(p));
            model=glm::rotate(model,float(astro::spinAngle(p,a.clock.jd)),glm::vec3(0,1,0));model=glm::scale(model,glm::vec3(r));
            shader.setMat4("model",model);glUniform1i(glGetUniformLocation(shader.ID,"isSun"),i==0);
            glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,resources.textures[i]);sphere.draw(GL_TRIANGLES);
            glm::vec4 clip=projection*view*glm::vec4(glm::vec3(pos),1);
            if(clip.w>0){auto n=glm::vec3(clip)/clip.w;if(std::abs(n.x)<.97 && std::abs(n.y)<.97 && n.z>-1 && n.z<1){
                a.screen[i]={sidebar+(n.x+1)*.5*(w-sidebar),(1-n.y)*.5*(h-footer)};a.visible[i]=true;}}
        }
        line.use();line.setMat4("view",view);
        for(int i=0;i<9;++i){
            glm::dvec3 pos=a.scale.position(a.planets[i],a.clock.jd)-target;
            glm::mat4 m=glm::translate(glm::mat4(1),glm::vec3(pos))*glm::mat4(astro::axisOrientation(a.planets[i]));
            m=glm::scale(m,glm::vec3(a.scale.radius(a.planets[i])));line.setMat4("model",m);
            if(a.axes){line.setVec3("lineColor",{.25f,.65f,.6f});axis.draw(GL_LINES);}
            if(i==6){line.setVec3("lineColor",{.7f,.6f,.4f});for(int j=0;j<3;++j)rings.draw(GL_LINE_LOOP,j*180,180);}
        }
        glViewport(0,0,fw,fh);hud.clear();
        if(a.labels)for(int i=0;i<9;++i)if(a.visible[i]){
            auto p=a.screen[i];if(p.y<180 || p.y>h-footer-18)continue;
            hud.rect(float(p.x)-2,float(p.y)-2,4,4,a.planets[i].color);
            if(p.x+90<w)hud.text(float(p.x)+8,float(p.y)-4,a.planets[i].name,a.planets[i].color,1.2f);
        }
        panel(hud,a,w,h);hud.draw(w,h);
        if(!shot.empty() && frames==2)capture(shot,fw,fh,!window);
        GLenum error=glGetError();if(error!=GL_NO_ERROR)throw std::runtime_error("OpenGL error "+std::to_string(error));
        if(window)glfwSwapBuffers(window);
        ++frames;if(frameLimit>0 && frames>=frameLimit)break;
    }
    std::cout<<"Rendered "<<frames<<" frames; no OpenGL errors.\n";return 0;
}
int main(int argc,char** argv){
    App a;std::string shot,exportPath;int frames=0;bool offscreen=false;
    try{
        for(int i=1;i<argc;++i){std::string arg=argv[i];auto value=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+arg);return std::string(argv[++i]);};
            if(arg=="--date"){a.dateInput=value();a.editingDate=true;applyDate(a);if(a.editingDate)throw std::runtime_error("Invalid --date");}
            else if(arg=="--jd"){
                auto input=value();size_t used=0;double date=std::stod(input,&used);
                if(used!=input.size())throw std::runtime_error("Invalid Julian date");
                (void)astro::elementsAt(a.planets[0],date);
                a.clock.setDate(date);
            }
            else if(arg=="--select"){int n=std::stoi(value());if(n<0||n>8)throw std::runtime_error("Select 0..8");a.focus(n);}
            else if(arg=="--true-scale")a.toggleScale();
            else if(arg=="--overview")a.reset(true);
            else if(arg=="--frames"){frames=std::stoi(value());if(frames<1)throw std::runtime_error("Frames must be positive");}
            else if(arg=="--offscreen")offscreen=true;
            else if(arg=="--screenshot")shot=value();
            else if(arg=="--export")exportPath=value();
            else if(arg=="--help"){std::cout<<"--offscreen | --date YYYY-MM-DD | --jd NUMBER | --select 0..8 | --true-scale | --overview | --frames N | --screenshot image.ppm | --export state.csv\n";return 0;}
            else throw std::runtime_error("Unknown argument "+arg);
        }
        if(!exportPath.empty()){exportCSV(a,exportPath);return 0;}
        if(!shot.empty() && frames>0 && frames<3)throw std::runtime_error("Screenshot requires at least 3 frames");
        if(offscreen){
            CGLPixelFormatAttribute attributes[]={kCGLPFAOpenGLProfile,(CGLPixelFormatAttribute)kCGLOGLPVersion_GL4_Core,kCGLPFAAccelerated,(CGLPixelFormatAttribute)0};
            CGLPixelFormatObj format=nullptr;GLint count=0;CGLContextObj context=nullptr;
            CGLError err=CGLChoosePixelFormat(attributes,&format,&count);
            if(err!=kCGLNoError || !format)throw std::runtime_error("Offscreen pixel format unavailable");
            err=CGLCreateContext(format,nullptr,&context);CGLDestroyPixelFormat(format);
            if(err!=kCGLNoError || !context)throw std::runtime_error("Offscreen OpenGL context unavailable");
            CGLSetCurrentContext(context);
            GLuint fbo=0,color=0,depth=0;glGenFramebuffers(1,&fbo);glBindFramebuffer(GL_FRAMEBUFFER,fbo);
            glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1280,800,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
            glGenRenderbuffers(1,&depth);glBindRenderbuffer(GL_RENDERBUFFER,depth);
            glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,1280,800);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
            int result=1;
            try{
                if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)throw std::runtime_error("Offscreen framebuffer incomplete");
                result=run(nullptr,a,shot,frames?frames:3);
            }catch(...){glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);CGLSetCurrentContext(nullptr);CGLDestroyContext(context);throw;}
            glDeleteRenderbuffers(1,&depth);glDeleteTextures(1,&color);glDeleteFramebuffers(1,&fbo);CGLSetCurrentContext(nullptr);CGLDestroyContext(context);
            return result;
        }
        glfwSetErrorCallback([](int code,const char* text){std::cerr<<"GLFW "<<code<<": "<<text<<'\n';});
        if(!glfwInit())throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);glfwWindowHint(GLFW_SAMPLES,4);
        GLFWwindow* w=glfwCreateWindow(1280,800,"Scientific Interactive 3D Solar System Simulator",nullptr,nullptr);
        if(!w){glfwTerminate();throw std::runtime_error("Cannot create OpenGL 4.1 core context");}
        glfwSetWindowSizeLimits(w,1050,780,GLFW_DONT_CARE,GLFW_DONT_CARE);
        glfwMakeContextCurrent(w);glfwSwapInterval(1);
        int result=0;try{result=run(w,a,shot,frames);}catch(...){glfwDestroyWindow(w);glfwTerminate();throw;}
        glfwDestroyWindow(w);glfwTerminate();return result;
    }catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<'\n';return 1;}
}
