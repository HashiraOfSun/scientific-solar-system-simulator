#include "Animation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace astro {
double radians(double deg) { return deg*pi/180.0; }
double solveKepler(double M, double e) {
    if (!std::isfinite(M) || !std::isfinite(e) || e < 0 || e >= 1)
        throw std::invalid_argument("Kepler requires finite M and 0 <= e < 1");
    M = std::remainder(M, 2*pi);
    double lo = -pi, hi = pi, E = e < 0.8 ? M : std::copysign(pi, M);
    // Safeguarded Newton iteration; monotone equation gives a reliable bracket.
    for (int i=0; i<100; ++i) {
        double f = E-e*std::sin(E)-M;
        if (std::abs(f) < 2e-14) return E;
        if (f > 0) hi=E; else lo=E;
        double next = E-f/(1-e*std::cos(E));
        E = next > lo && next < hi ? next : 0.5*(lo+hi);
    }
    throw std::runtime_error("Kepler solver did not converge");
}
Elements elementsAt(const Planet& p, double jd) {
    if (!std::isfinite(jd) || jd < minJD || jd > maxJD)
        throw std::out_of_range("JPL Table 1: date outside 1800-2050");
    double t = (jd-j2000)/36525.0;
    return {p.epoch.a+p.rate.a*t, p.epoch.e+p.rate.e*t,
            p.epoch.inclination+p.rate.inclination*t,
            p.epoch.longitude+p.rate.longitude*t,
            p.epoch.perihelion+p.rate.perihelion*t,
            p.epoch.node+p.rate.node*t};
}
glm::dvec3 orbitPoint(const Elements& e, double E) {
    double w=radians(e.perihelion-e.node), O=radians(e.node), I=radians(e.inclination);
    double x=e.a*(std::cos(E)-e.e), y=e.a*std::sqrt(1-e.e*e.e)*std::sin(E);
    return {(std::cos(w)*std::cos(O)-std::sin(w)*std::sin(O)*std::cos(I))*x+
            (-std::sin(w)*std::cos(O)-std::cos(w)*std::sin(O)*std::cos(I))*y,
            (std::cos(w)*std::sin(O)+std::sin(w)*std::cos(O)*std::cos(I))*x+
            (-std::sin(w)*std::sin(O)+std::cos(w)*std::cos(O)*std::cos(I))*y,
            std::sin(w)*std::sin(I)*x+std::cos(w)*std::sin(I)*y};
}
glm::dvec3 positionAU(const Planet& p, double jd) {
    auto e=elementsAt(p,jd);
    if (e.a==0) return glm::dvec3(0);
    return orbitPoint(e,solveKepler(radians(e.longitude-e.perihelion),e.e));
}
glm::dvec3 toScene(const glm::dvec3& v) { return {v.x,v.z,-v.y}; }
glm::dmat4 axisOrientation(const Planet& p) {
    // Educational obliquity model, fixed at J2000. Pole azimuth is illustrative.
    auto m=glm::rotate(glm::dmat4(1), radians(p.epoch.node), glm::dvec3(0,1,0));
    m=glm::rotate(m,radians(p.epoch.inclination),glm::dvec3(1,0,0));
    return glm::rotate(m,radians(p.obliquityDeg),glm::dvec3(0,0,1));
}
double spinAngle(const Planet& p, double jd) {
    // Retrograde is already encoded by obliquity > 90 deg: do not negate twice.
    return 2*pi*std::remainder((jd-j2000)*24/std::abs(p.rotationHours),1.0);
}
double speedKmS(const Planet& p, double jd) {
    if (p.epoch.a==0) return 0;
    double r=glm::length(positionAU(p,jd))*auKm, a=elementsAt(p,jd).a*auKm;
    return std::sqrt(solarGM*(2/r-1/a));
}
double periodDays(const Planet& p, double jd) {
    double a=elementsAt(p,jd).a*auKm;
    return 2*pi*std::sqrt(a*a*a/solarGM)/86400;
}
double julianDate(int year,int month,int day) {
    if (year<1800 || year>2050 || month<1 || month>12) throw std::invalid_argument("Invalid date");
    bool leap=year%4==0 && (year%100!=0 || year%400==0);
    const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(day<1 || day>days[month-1]+(month==2 && leap)) throw std::invalid_argument("Invalid day");
    if(month<=2){--year;month+=12;}
    int a=year/100, b=2-a+a/4;
    double jd=std::floor(365.25*(year+4716))+std::floor(30.6001*(month+1))+day+b-1524.5;
    if(jd<minJD || jd>maxJD) throw std::out_of_range("Date outside model range");
    return jd;
}
std::string calendarDate(double jd) {
    double z=std::floor(jd+0.5), f=jd+0.5-z;
    double alpha=std::floor((z-1867216.25)/36524.25);
    double a=z+1+alpha-std::floor(alpha/4), b=a+1524;
    double c=std::floor((b-122.1)/365.25), d=std::floor(365.25*c), e=std::floor((b-d)/30.6001);
    int day=int(b-d-std::floor(30.6001*e)), month=int(e<14?e-1:e-13);
    int year=int(month>2?c-4716:c-4715);
    int minute=std::min(1439,int(f*1440+1e-6));
    char s[40]; std::snprintf(s,sizeof(s),"%04d-%02d-%02d %02d:%02d TDB",year,month,day,minute/60,minute%60);
    return s;
}
void SimulationClock::setDate(double date) {
    if(!std::isfinite(date)) throw std::invalid_argument("Nonfinite date");
    jd=std::clamp(date,minJD,maxJD);
    if(date<=minJD || date>=maxJD) paused=true;
}
void SimulationClock::step(double days) { setDate(jd+days); }
void SimulationClock::advance(double seconds) {
    if(!std::isfinite(seconds) || seconds<0 || !std::isfinite(daysPerSecond))
        throw std::invalid_argument("Invalid clock input");
    if(!paused) step(seconds*daysPerSecond);
}
double DisplayScale::radius(const Planet& p) const {
    double boost = p.epoch.a==0 ? radiusExaggeration/50.0 : radiusExaggeration;
    return p.radiusKm/auKm*unitsPerAU*(trueScale?1:boost);
}
glm::dvec3 DisplayScale::position(const Planet& p,double jd) const {
    return toScene(positionAU(p,jd))*unitsPerAU;
}
}
