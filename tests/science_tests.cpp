#include "Animation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace astro;
int checks=0;
void require(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void close(double a,double b,double tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
int main(){try{
    auto planets=createPlanets();
    for(double e:{0.,.0167,.2056,.8,.95,.999999})for(double m:{-1000.,-pi,-.01,0.,.01,1.,pi,1000.}){
        double E=solveKepler(m,e);close(E-e*std::sin(E),std::remainder(m,2*pi),3e-14,"Kepler residual");
    }
    for(double e:{-1.,1.,std::numeric_limits<double>::quiet_NaN()}){
        bool threw=false;try{solveKepler(1,e);}catch(const std::invalid_argument&){threw=true;}require(threw,"Reject invalid eccentricity");
    }
    Elements e{2,.2,0,0,0,0};
    close(glm::length(orbitPoint(e,0)),1.6,1e-14,"Perihelion a(1-e)");
    close(glm::length(orbitPoint(e,pi)),2.4,1e-14,"Aphelion a(1+e)");
    e.node=90;e.perihelion=90;auto pos=orbitPoint(e,0);close(pos.x,0,1e-14,"Node X");close(pos.y,1.6,1e-14,"Node Y");
    e.node=0;e.perihelion=0;e.inclination=90;pos=orbitPoint(e,pi/2);close(pos.y,0,1e-14,"Inclination Y");require(pos.z>1.9,"Inclination Z");
    close(glm::dot(toScene({1,0,0}),glm::cross(toScene({0,1,0}),toScene({0,0,1}))),1,1e-15,"Right handed scene transform");
    for(size_t i=1;i<planets.size();++i){auto p=planets[i];
        for(double jd:{minJD,j2000,julianDate(2026,10,21),maxJD}){
            auto el=elementsAt(p,jd);auto r=positionAU(p,jd);double distance=glm::length(r);
            require(std::isfinite(distance),"Finite position");require(distance>=el.a*(1-el.e)-1e-12 && distance<=el.a*(1+el.e)+1e-12,"Orbital radius bounds");
            double I=radians(el.inclination),O=radians(el.node);glm::dvec3 normal(std::sin(O)*std::sin(I),-std::cos(O)*std::sin(I),std::cos(I));
            close(glm::dot(r,normal),0,1e-12,"Position lies in inclined plane");
            double period=periodDays(p,jd)*86400;double a=el.a*auKm;
            close(period*period/(a*a*a),4*pi*pi/solarGM,1e-23,"Kepler third law");
        }
        // Hold fitted elements fixed to test pure Kepler laws, not secular drift.
        auto el=p.epoch;double areaScale=.5*el.a*el.a*std::sqrt(1-el.e*el.e);
        double firstArea=0;
        for(int j=0;j<8;++j){double m0=-2.8+j*.7,m1=m0+.2,E0=solveKepler(m0,el.e),E1=solveKepler(m1,el.e);
            double area=areaScale*((E1-el.e*std::sin(E1))-(E0-el.e*std::sin(E0)));
            if(j==0)firstArea=area;else close(area,firstArea,1e-10,"Equal areas in equal time");}
        p.rate={0,0,0,0,0,0};p.epoch.longitude=p.epoch.perihelion;
        double vp=speedKmS(p,j2000);p.epoch.longitude+=180;double va=speedKmS(p,j2000);
        require(vp>va,"Faster at perihelion");close(vp/va,(1+el.e)/(1-el.e),1e-12,"Apsidal speed ratio");
        auto axis=glm::dvec3(axisOrientation(p)*glm::dvec4(0,1,0,0));
        double I=radians(p.epoch.inclination),O=radians(p.epoch.node);
        auto normal=toScene({std::sin(O)*std::sin(I),-std::cos(O)*std::sin(I),std::cos(I)});
        close(glm::dot(axis,normal),std::cos(radians(p.obliquityDeg)),1e-12,"Axial obliquity");
        if(i==2||i==7)require(glm::dot(axis,normal)<0,"Retrograde direction without double reversal");
    }
    close(julianDate(2000,1,1)+.5,j2000,0,"J2000 noon");
    require(calendarDate(j2000)=="2000-01-01 12:00 TDB","Calendar J2000");
    require(calendarDate(minJD)=="1800-01-01 00:00 TDB","Calendar lower bound");
    require(calendarDate(maxJD)=="2050-01-01 00:00 TDB","Calendar upper bound");
    require(calendarDate(julianDate(2000,2,29))=="2000-02-29 00:00 TDB","Leap day");
    bool invalid=false;try{julianDate(1900,2,29);}catch(...){invalid=true;}require(invalid,"Reject 1900 leap day");
    invalid=false;try{positionAU(planets[3],maxJD+1);}catch(...){invalid=true;}require(invalid,"Reject out of range ephemeris");
    SimulationClock c; c.advance(10);close(c.jd,j2000,0,"Paused clock");c.paused=false;c.advance(3);close(c.jd,j2000+30,0,"Simulation days per second");c.daysPerSecond=-10;c.advance(3);close(c.jd,j2000,0,"Reverse time");
    SimulationClock c1,c2;c1.paused=c2.paused=false;
    for(int i=0;i<60;++i)c1.advance(1./60);for(int i=0;i<144;++i)c2.advance(1./144);
    close(c1.jd,c2.jd,1e-7,"Frame rate independence");
    c.setDate(maxJD-1);c.paused=false;c.daysPerSecond=10;c.advance(1);close(c.jd,maxJD,0,"Clamp upper boundary");require(c.paused,"Pause at boundary");
    c.setDate(minJD+1);c.paused=false;c.daysPerSecond=-10;c.advance(1);close(c.jd,minJD,0,"Clamp lower boundary");require(c.paused,"Pause lower boundary");
    DisplayScale scale;auto before=positionAU(planets[3],j2000);double r=scale.radius(planets[3]);scale.radiusExaggeration*=2;close(scale.radius(planets[3]),2*r,1e-14,"Independent radius scale");
    close(glm::length(positionAU(planets[3],j2000)-before),0,0,"Display does not affect physics");scale.trueScale=true;
    close(scale.radius(planets[3])/scale.unitsPerAU,planets[3].radiusKm/auKm,1e-18,"True physical scale");
    std::cout<<"PASS: "<<checks<<" scientific checks\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
